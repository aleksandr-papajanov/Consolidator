#include "Core/Routing/StateWriter.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

#include "Core/Domain/State/InstanceState.h"
#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

namespace
{

std::optional<dsp::ParameterVariant> ToParameterVariant(
    const StateValue& value)
{
    return std::visit(
        [](const auto& typedValue) -> std::optional<dsp::ParameterVariant>
        {
            using ValueType = std::decay_t<decltype(typedValue)>;
            if constexpr (std::is_same_v<ValueType, bool> ||
                          std::is_same_v<ValueType, std::int32_t> ||
                          std::is_same_v<ValueType, float>)
            {
                return dsp::ParameterVariant{typedValue};
            }
            return std::nullopt;
        },
        value);
}

void AppendUnique(
    std::vector<BankAddress>& banks,
    BankAddress bank)
{
    if (std::find(banks.begin(), banks.end(), bank) == banks.end())
    {
        banks.push_back(bank);
    }
}

std::vector<StatePath> BuildConstraintRefreshPaths(
    const std::vector<BankAddress>& affectedBanks)
{
    std::vector<StatePath> paths;
    for (const auto& bank : affectedBanks)
    {
        auto path = StatePath::Instance(bank.instanceId);
        path.field = StateField::DspParameter;
        if (std::find(paths.begin(), paths.end(), path) == paths.end())
        {
            paths.push_back(path);
        }
    }
    return paths;
}

} // namespace

StateWriter::StateWriter(
    InstanceRegistry& registry,
    const StateRouter& stateRouter,
    const ParameterConstraintResolver& constraintResolver) noexcept
    : registry_(registry)
    , stateRouter_(stateRouter)
    , constraintResolver_(constraintResolver)
{
}

StateResponse StateWriter::Write(
    const WriteStateCommand& command)
{
    WriteContext context{
        .response = StateResponse{
            command.requestId,
            command.instanceId,
            {}}};

    ApplyEntries(command.instanceId, command, context);
    PublishDspUpdates(context);
    RefreshConstraints(context);
    return FinalizeResponse(context);
}

void StateWriter::ApplyEntries(
    InstanceId sourceInstanceId,
    const WriteStateCommand& command,
    WriteContext& context)
{
    for (std::size_t index = 0;
         index < command.entries.size;
         ++index)
    {
        ApplyEntry(
            sourceInstanceId,
            command.entries.entries[index],
            context);
    }
}

void StateWriter::ApplyEntry(
    InstanceId sourceInstanceId,
    const StateEntry& entry,
    WriteContext& context)
{
    if (TryApplyTopology(sourceInstanceId, entry, context))
    {
        return;
    }

    if (!constraintResolver_.Validate(sourceInstanceId, entry))
    {
        AddRejected(entry, context);
        return;
    }

    const auto targets = stateRouter_.ResolveWriteTargets(sourceInstanceId, entry.path);
    if (targets.empty())
    {
        if (!ApplyToInstance(sourceInstanceId, entry, context))
        {
            AddRejected(entry, context);
        }
        return;
    }

    ApplyToTargets(sourceInstanceId, entry, targets, context);
}

bool StateWriter::TryApplyTopology(
    InstanceId sourceInstanceId,
    const StateEntry& entry,
    WriteContext& context)
{
    auto* source = registry_.FindInstance(sourceInstanceId);
    if (source == nullptr || !entry.path.field)
    {
        return false;
    }

    std::optional<BankAddress> changedBank;
    std::optional<GroupId> previousGroup;
    if (*entry.path.field == StateField::GroupId)
    {
        const auto bankId = entry.path.TryGetBankId();
        if (!bankId)
        {
            return false;
        }

        changedBank = BankAddress{sourceInstanceId, *bankId};
        const auto& sourceState = source->GetStateStore().GetInstance();
        previousGroup = sourceState.banks[dsp::detail::ToIndex(*bankId)].groupId;
    }

    StateResponseEntries applied;
    const auto status = source->GetStateStore().WriteState(
        entry,
        applied);
    switch (status)
    {
    case StateWriteStatus::NotHandled:
        return false;

    case StateWriteStatus::Rejected:
        AddRejected(entry, context);
        return true;

    case StateWriteStatus::Unchanged:
        AppendApplied(applied, context);
        return true;

    case StateWriteStatus::Applied:
        AppendApplied(applied, context);
        break;
    }

    if (!changedBank)
    {
        return true;
    }

    std::vector<BankAddress> affectedBanks;
    if (previousGroup)
    {
        for (const auto& member : registry_.FindGroupMembers(*previousGroup))
        {
            AppendUnique(affectedBanks, member);
        }
    }

    const auto& sourceState = source->GetStateStore().GetInstance();
    const auto nextGroup = sourceState.banks[
        dsp::detail::ToIndex(changedBank->bankId)].groupId;
    if (nextGroup)
    {
        for (const auto& member : registry_.FindGroupMembers(*nextGroup))
        {
            AppendUnique(affectedBanks, member);
        }
    }

    AppendUnique(affectedBanks, *changedBank);
    registry_.CacheBankGroup(*changedBank, previousGroup, nextGroup);
    for (const auto& path : BuildConstraintRefreshPaths(affectedBanks))
    {
        if (std::find(context.constraintPaths.begin(), context.constraintPaths.end(), path) ==
            context.constraintPaths.end())
        {
            context.constraintPaths.push_back(path);
        }
    }
    return true;
}

bool StateWriter::ApplyToInstance(
    InstanceId targetInstanceId,
    const StateEntry& entry,
    WriteContext& context)
{
    auto* target = registry_.FindInstance(targetInstanceId);
    if (target == nullptr)
    {
        return false;
    }

    StateResponseEntries applied;
    const auto status = target->GetStateStore().WriteState(entry, applied);
    std::optional<StateEntry> appliedParameter;
    for (std::size_t index = 0; index < applied.size; ++index)
    {
        if (applied.entries[index].path == entry.path)
        {
            appliedParameter = applied.entries[index];
        }
    }

    switch (status)
    {
    case StateWriteStatus::NotHandled:
        return false;

    case StateWriteStatus::Rejected:
        AddRejected(entry, context);
        return true;

    case StateWriteStatus::Unchanged:
        AppendApplied(applied, context);
        return true;

    case StateWriteStatus::Applied:
        AppendApplied(applied, context);
        if (!appliedParameter)
        {
            CollectConstraintPaths(targetInstanceId, entry, context);
            return true;
        }
        if (const auto parameterValue = ToParameterVariant(appliedParameter->value))
        {
            context.dspUpdates.push_back({
                targetInstanceId,
                DspUpdate{appliedParameter->path, *parameterValue, 0}});
        }
        CollectConstraintPaths(targetInstanceId, entry, context);
        return true;
    }

    return false;
}

void StateWriter::ApplyToTargets(
    InstanceId sourceInstanceId,
    const StateEntry& entry,
    const std::vector<BankAddress>& targets,
    WriteContext& context)
{
    std::vector<std::pair<InstanceId, StateEntry>> targetEntries;
    targetEntries.reserve(targets.size());
    for (const auto& target : targets)
    {
        auto targetEntry = constraintResolver_.TranslateForTarget(
            sourceInstanceId,
            entry,
            target);
        if (!targetEntry)
        {
            AddRejected(entry, context);
            return;
        }

        targetEntry->path = StateRouter::Retarget(
            std::move(targetEntry->path),
            target);

        auto* targetInstance = registry_.FindInstance(target.instanceId);
        if (targetInstance == nullptr ||
            !targetInstance->GetStateStore().CanWrite(*targetEntry))
        {
            targetEntry->status = StateWriteStatus::Rejected;
            (void)context.response.entries.TryAppend(std::move(*targetEntry));
            return;
        }

        targetEntries.emplace_back(target.instanceId, std::move(*targetEntry));
    }

    for (auto& targetEntry : targetEntries)
    {
        const bool applied = ApplyToInstance(
            targetEntry.first,
            targetEntry.second,
            context);
        assert(applied && "validated target must be writable");
    }
}

void StateWriter::CollectConstraintPaths(
    InstanceId targetInstanceId,
    const StateEntry& entry,
    WriteContext& context)
{
    for (const auto& dependency : stateRouter_.ResolveConstraintTargets(
             targetInstanceId,
             entry.path))
    {
        auto path = StateRouter::Retarget(entry.path, dependency);

        if (std::find(context.constraintPaths.begin(), context.constraintPaths.end(), path) ==
            context.constraintPaths.end())
        {
            context.constraintPaths.push_back(path);
        }
    }
}

void StateWriter::AddRejected(
    const StateEntry& entry,
    WriteContext& context) const
{
    auto rejected = entry;
    rejected.status = StateWriteStatus::Rejected;
    (void)context.response.entries.TryAppend(std::move(rejected));
}

void StateWriter::AppendApplied(
    StateResponseEntries& applied,
    WriteContext& context) const
{
    for (std::size_t index = 0; index < applied.size; ++index)
    {
        (void)context.response.entries.TryAppend(std::move(applied.entries[index]));
    }
}

void StateWriter::PublishDspUpdates(WriteContext& context)
{
    std::vector<InstanceId> publishedInstances;
    std::vector<DspUpdate> updates;
    for (const auto& pending : context.dspUpdates)
    {
        if (std::find(publishedInstances.begin(), publishedInstances.end(), pending.instanceId) !=
            publishedInstances.end())
        {
            continue;
        }

        auto* instance = registry_.FindInstance(pending.instanceId);
        if (instance == nullptr)
        {
            continue;
        }

        updates.clear();
        for (const auto& candidate : context.dspUpdates)
        {
            if (candidate.instanceId == pending.instanceId)
            {
                updates.push_back(candidate.update);
            }
        }
        instance->PublishDspUpdates(std::span<const DspUpdate>{updates.data(), updates.size()});
        publishedInstances.push_back(pending.instanceId);
    }
}

void StateWriter::RefreshConstraints(WriteContext& context)
{
    for (const auto& path : context.constraintPaths)
    {
        if (!path.instanceId)
        {
            continue;
        }

        auto* instance = registry_.FindInstance(*path.instanceId);
        if (instance == nullptr)
        {
            continue;
        }

        StateResponseEntries refreshed;
        instance->GetStateStore().ReadState(path, refreshed);
        for (std::size_t index = 0; index < refreshed.size; ++index)
        {
            const auto& refreshedEntry = refreshed.entries[index];
            const auto begin = context.response.entries.entries.begin();
            const auto end = begin + context.response.entries.size;
            const auto existing = std::find_if(
                begin,
                end,
                [&refreshedEntry](const StateEntry& candidate)
                {
                    return candidate.path == refreshedEntry.path;
                });
            if (existing == end)
            {
                (void)context.response.entries.TryAppend(refreshedEntry);
            }
        }
    }
}

StateResponse StateWriter::FinalizeResponse(WriteContext& context)
{
    for (std::size_t index = 0; index < context.response.entries.size; ++index)
    {
        auto& entry = context.response.entries.entries[index];
        const auto instanceId = entry.path.instanceId.value_or(
            context.response.instanceId);
        constraintResolver_.Enrich(instanceId, entry);
    }

    context.response.truncated = context.response.entries.truncated;
    return std::move(context.response);
}

} // namespace consolidator::core

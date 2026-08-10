#include "Core/Routing/CommandRouter.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "Core/Routing/StateResponsePublisher.h"
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

std::optional<dsp::BankId> TryGetBankId(
    const StatePath& path) noexcept
{
    if (path.depth == 0)
    {
        return std::nullopt;
    }

    const auto node =
        static_cast<std::uint8_t>(path.nodes[0]);
    const auto first =
        static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0);
    const auto last =
        first + InstanceState::kBankCount - 1;

    if (node < first || node > last)
    {
        return std::nullopt;
    }

    return static_cast<dsp::BankId>(node - first);
}

} // namespace

void CommandRouter::HandleWriteCommand(
    InstanceId sourceInstanceId,
    const StateCommand& command)
{
    auto plan = BuildWritePlan(sourceInstanceId, command);
    PublishResponse(std::move(plan));
}

CommandRouter::WritePlan CommandRouter::BuildWritePlan(
    InstanceId sourceInstanceId,
    const StateCommand& command)
{
    WritePlan plan{
        .coordinatorResponse = StateResponse{
            command.message.requestId,
            command.message.responseInstanceId,
            sourceInstanceId,
            command.operation,
            {}}};

    for (std::size_t index = 0;
         index < command.message.entries.size;
         ++index)
    {
        RouteWriteEntry(
            sourceInstanceId,
            command.message.entries.entries[index],
            plan);
    }

    return plan;
}

bool CommandRouter::ApplyTopologyWrite(
    InstanceId sourceInstanceId,
    const StateEntry& entry,
    StateResponseEntries& applied,
    std::vector<BankAddress>& affectedBanks)
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
        const auto bankId = TryGetBankId(entry.path);
        if (!bankId)
        {
            return false;
        }

        changedBank = BankAddress{sourceInstanceId, *bankId};
        const auto& sourceState =
            static_cast<const InstanceState&>(
                source->GetStateStore().GetTopology());
        previousGroup = sourceState.GetBankState(*bankId).GetGroupId();
    }

    const auto status =
        source->GetStateStore().GetTopology().WriteState(entry, applied);
    if (status == StateWriteStatus::NotHandled)
    {
        return false;
    }

    if (status == StateWriteStatus::Rejected)
    {
        auto rejected = entry;
        rejected.status = StateWriteStatus::Rejected;
        (void)applied.TryAppend(std::move(rejected));
        return true;
    }

    if (changedBank)
    {
        const auto appendUnique = [&affectedBanks](BankAddress bank)
        {
            if (std::find(
                    affectedBanks.begin(),
                    affectedBanks.end(),
                    bank) == affectedBanks.end())
            {
                affectedBanks.push_back(bank);
            }
        };

        if (previousGroup)
        {
            for (const auto& member : registry_.FindGroupMembers(*previousGroup))
            {
                appendUnique(member);
            }
        }

        const auto& sourceState =
            static_cast<const InstanceState&>(
                source->GetStateStore().GetTopology());
        const auto nextGroup =
            sourceState.GetBankState(changedBank->bankId).GetGroupId();

        if (nextGroup)
        {
            for (const auto& member : registry_.FindGroupMembers(*nextGroup))
            {
                appendUnique(member);
            }
        }

        appendUnique(*changedBank);
        registry_.CacheBankGroup(*changedBank, previousGroup, nextGroup);
    }

    return true;
}

bool CommandRouter::ApplyStateStoreWrite(
    InstanceId targetInstanceId,
    const StateEntry& entry,
    WritePlan& plan)
{
    auto* target = registry_.FindInstance(targetInstanceId);
    if (target == nullptr)
    {
        return false;
    }

    StateResponseEntries applied;
    const auto status = target->GetStateStore().WriteState(entry, applied);
    if (status == StateWriteStatus::NotHandled)
    {
        return false;
    }

    std::optional<StateEntry> appliedParameter;
    for (std::size_t index = 0; index < applied.size; ++index)
    {
        if (applied.entries[index].path == entry.path)
        {
            appliedParameter = applied.entries[index];
        }
        (void)plan.coordinatorResponse.entries.TryAppend(
            std::move(applied.entries[index]));
    }

    if (status == StateWriteStatus::Applied && appliedParameter)
    {
        if (const auto parameterValue =
                ToParameterVariant(appliedParameter->value))
        {
            plan.pendingDspUpdates.push_back(
                WritePlan::PendingDspUpdate{
                    targetInstanceId,
                    DspUpdate{
                        appliedParameter->path,
                        *parameterValue,
                        0}});
        }
    }

    return true;
}

void CommandRouter::CollectConstraintDependencyPaths(
    InstanceId targetInstanceId,
    const StateEntry& entry,
    WritePlan& plan)
{
    const auto dependencies =
        stateRouter_.ResolveConstraintDependencies(
            targetInstanceId,
            entry.path);

    for (const auto& dependency : dependencies)
    {
        auto query = entry.path;
        query.instanceId = dependency.instanceId;
        if (StateRouter::IsBankOwned(entry.path))
        {
            query.depth = 1;
            query.nodes[0] = static_cast<dsp::RouteNodeId>(
                static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) +
                dsp::detail::ToIndex(dependency.bankId));
        }

        const auto alreadyIncluded = std::find(
            plan.affectedConstraintPaths.begin(),
            plan.affectedConstraintPaths.end(),
            query);
        if (alreadyIncluded == plan.affectedConstraintPaths.end())
        {
            plan.affectedConstraintPaths.push_back(query);
        }
    }
}

void CommandRouter::RouteWriteEntry(
    InstanceId sourceInstanceId,
    const StateEntry& entry,
    WritePlan& plan)
{
    if (entry.path.field == StateField::DspParameter &&
        !constraintResolver_.Validate(sourceInstanceId, entry))
    {
        auto rejected = entry;
        rejected.status = StateWriteStatus::Rejected;
        (void)plan.coordinatorResponse.entries.TryAppend(std::move(rejected));
        return;
    }

    std::vector<BankAddress> affectedBanks;
    if (ApplyTopologyWrite(
            sourceInstanceId,
            entry,
            plan.coordinatorResponse.entries,
            affectedBanks))
    {
        if (entry.path.field != StateField::GroupId)
        {
            return;
        }

        for (const auto& dependency :
             stateRouter_.ResolveTopologyConstraintDependencies(affectedBanks))
        {
            if (std::find(
                    plan.affectedConstraintPaths.begin(),
                    plan.affectedConstraintPaths.end(),
                    dependency) == plan.affectedConstraintPaths.end())
            {
                plan.affectedConstraintPaths.push_back(dependency);
            }
        }
        return;
    }

    const auto targets =
        stateRouter_.ResolveWriteTargets(sourceInstanceId, entry.path);
    if (targets.empty())
    {
        if (ApplyStateStoreWrite(sourceInstanceId, entry, plan))
        {
            CollectConstraintDependencyPaths(sourceInstanceId, entry, plan);
            return;
        }

        auto rejected = entry;
        rejected.status = StateWriteStatus::Rejected;
        (void)plan.coordinatorResponse.entries.TryAppend(std::move(rejected));
        return;
    }

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
            auto rejected = entry;
            rejected.status = StateWriteStatus::Rejected;
            (void)plan.coordinatorResponse.entries.TryAppend(std::move(rejected));
            return;
        }

        targetEntry->path.instanceId = target.instanceId;
        if (StateRouter::IsBankOwned(entry.path))
        {
            targetEntry = StateRouter::ForBank(
                std::move(*targetEntry),
                target.bankId);
        }

        auto* targetInstance = registry_.FindInstance(target.instanceId);
        if (targetInstance == nullptr ||
            !targetInstance->GetStateStore().CanWrite(*targetEntry))
        {
            auto rejected = *targetEntry;
            rejected.status = StateWriteStatus::Rejected;
            (void)plan.coordinatorResponse.entries.TryAppend(std::move(rejected));
            return;
        }

        targetEntries.emplace_back(target.instanceId, std::move(*targetEntry));
    }

    for (auto& targetEntry : targetEntries)
    {
        const bool applied = ApplyStateStoreWrite(
            targetEntry.first,
            targetEntry.second,
            plan);
        assert(applied && "validated target must be writable");
    }

    for (const auto& targetEntry : targetEntries)
    {
        CollectConstraintDependencyPaths(
            targetEntry.first,
            targetEntry.second,
            plan);
    }
}

void CommandRouter::PublishResponse(WritePlan plan)
{
    PublishDspUpdates(plan);
    RefreshConstraintEntries(plan);
    PublishStateResponse(std::move(plan));
}

void CommandRouter::PublishDspUpdates(WritePlan& plan)
{
    std::vector<InstanceId> publishedInstances;
    std::vector<DspUpdate> updates;
    for (const auto& pending : plan.pendingDspUpdates)
    {
        if (std::find(
                publishedInstances.begin(),
                publishedInstances.end(),
                pending.instanceId) != publishedInstances.end())
        {
            continue;
        }

        auto* instance = registry_.FindInstance(pending.instanceId);
        if (instance == nullptr)
        {
            continue;
        }

        updates.clear();
        for (const auto& candidate : plan.pendingDspUpdates)
        {
            if (candidate.instanceId == pending.instanceId)
            {
                updates.push_back(candidate.update);
            }
        }

        instance->PublishDspUpdates(
            std::span<const DspUpdate>{updates.data(), updates.size()});
        publishedInstances.push_back(pending.instanceId);
    }
}

void CommandRouter::RefreshConstraintEntries(WritePlan& plan)
{
    for (const auto& path : plan.affectedConstraintPaths)
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
            const auto alreadyIncluded = std::find_if(
                plan.coordinatorResponse.entries.entries.begin(),
                plan.coordinatorResponse.entries.entries.begin() +
                    plan.coordinatorResponse.entries.size,
                [&refreshedEntry](const StateEntry& existing)
                {
                    return existing.path == refreshedEntry.path;
                });
            if (alreadyIncluded ==
                    plan.coordinatorResponse.entries.entries.begin() +
                        plan.coordinatorResponse.entries.size)
            {
                (void)plan.coordinatorResponse.entries.TryAppend(refreshedEntry);
            }
        }
    }
}

void CommandRouter::PublishStateResponse(WritePlan plan)
{
    for (std::size_t index = 0;
         index < plan.coordinatorResponse.entries.size;
         ++index)
    {
        auto& entry = plan.coordinatorResponse.entries.entries[index];
        const auto instanceId = entry.path.instanceId.value_or(
            plan.coordinatorResponse.appliedInstanceId);
        constraintResolver_.Enrich(instanceId, entry);
    }

    StateResponsePublisher publisher{
        coordinatorResponses_,
        1};
    publisher.Publish(std::move(plan.coordinatorResponse), 0);
}

} // namespace consolidator::core

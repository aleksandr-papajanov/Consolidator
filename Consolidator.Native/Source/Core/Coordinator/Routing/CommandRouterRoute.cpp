#include "Core/Coordinator/Routing/CommandRouter.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

namespace
{

std::optional<dsp::ParameterValue> ToParameterValue(const StateValue& value)
{
    return std::visit(
        [](const auto& typedValue) -> std::optional<dsp::ParameterValue>
        {
            using ValueType = std::decay_t<decltype(typedValue)>;
            if constexpr (std::is_same_v<ValueType, bool> ||
                          std::is_same_v<ValueType, std::int32_t> ||
                          std::is_same_v<ValueType, float>)
            {
                return dsp::ParameterValue{typedValue};
            }
            return std::nullopt;
        },
        value);
}

} // namespace

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
        if (const auto parameterValue = ToParameterValue(appliedParameter->value))
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
        stateRouter_.ResolveWriteTargets(
            sourceInstanceId,
            entry.path);

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

} // namespace consolidator::core

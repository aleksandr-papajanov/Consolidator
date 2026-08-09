#include "Core/Coordinator/Routing/CommandRouter.h"

#include <utility>


namespace consolidator::core
{

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
        (void)plan.topologyResponse.entries.TryAppend(std::move(rejected));
        return;
    }

    std::vector<BankAddress> affectedBanks;
    if (ApplyTopologyWrite(
            sourceInstanceId,
            entry,
            plan.topologyResponse.entries,
            affectedBanks))
    {
        if (entry.path.field != StateField::GroupId)
        {
            return;
        }
        // Topology edits can invalidate a whole group's effective ranges.
        // Ask only the old and new group members for their normal state;
        // InstanceCoordinator enriches those entries through the same resolver.
        for (const auto& member : affectedBanks)
        {
            auto& batch = GetOrCreateBatch(plan, member.instanceId);
            batch.command.operation = StateOperation::Read;
            auto query = StatePath::Instance(member.instanceId);
            static_cast<void>(batch.command.message.entries.TryAppend(
                StateEntry{query, StateValue{std::monostate{}}}));
        }
        return;
    }

    const auto targets =
        stateRouter_.ResolveTargets(
            sourceInstanceId,
            entry.path);

    if (targets.empty())
    {
        auto& batch =
            GetOrCreateBatch(
                plan,
                sourceInstanceId);

        static_cast<void>(batch.command.message.entries.TryAppend(entry));

        return;
    }

    for (const auto& target : targets)
    {
        auto& batch =
            GetOrCreateBatch(
                plan,
                target.instanceId);

        auto targetEntry = constraintResolver_.TranslateForTarget(
            sourceInstanceId,
            entry,
            target);
        if (!targetEntry)
        {
            auto rejected = entry;
            rejected.status = StateWriteStatus::Rejected;
            (void)plan.topologyResponse.entries.TryAppend(std::move(rejected));
            return;
        }
        if (StateRouter::IsBankOwned(entry.path))
        {
            targetEntry = StateRouter::ForBank(
                std::move(*targetEntry),
                target.bankId);
        }
        static_cast<void>(batch.command.message.entries.TryAppend(
            std::move(*targetEntry)));
    }
}

} // namespace consolidator::core

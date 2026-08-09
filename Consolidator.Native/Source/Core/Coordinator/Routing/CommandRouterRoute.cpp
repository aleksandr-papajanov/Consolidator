#include "Core/Coordinator/Routing/CommandRouter.h"

namespace consolidator::core
{

void CommandRouter::RouteWriteEntry(
    InstanceId sourceInstanceId,
    const StateEntry& entry,
    WritePlan& plan)
{
    if (ApplyTopologyWrite(
            sourceInstanceId,
            entry,
            plan.topologyResponse.entries))
    {
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

        static_cast<void>(
            batch.command.message.entries.TryAppend(entry));

        return;
    }

    for (const auto& target : targets)
    {
        auto& batch =
            GetOrCreateBatch(
                plan,
                target.instanceId);

        static_cast<void>(
            batch.command.message.entries.TryAppend(
                StateRouter::ForBank(
                    entry,
                    target.bankId)));
    }
}

} // namespace consolidator::core

#include "Core/Coordinator/Routing/CommandRouter.h"

#include <cstddef>
#include <algorithm>
#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"
#include "Core/Coordinator/Routing/StateResponsePublisher.h"

namespace consolidator::core
{
void CommandRouter::PublishResponse(
    WritePlan plan)
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
            if (alreadyIncluded == plan.coordinatorResponse.entries.entries.begin() +
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

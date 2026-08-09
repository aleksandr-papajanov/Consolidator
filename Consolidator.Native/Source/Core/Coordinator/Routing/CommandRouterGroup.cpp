#include "Core/Coordinator/Routing/CommandRouter.h"

#include <algorithm>

namespace consolidator::core
{

CommandRouter::RoutedBatch&
CommandRouter::GetOrCreateBatch(
    WritePlan& plan,
    InstanceId instanceId)
{
    const auto existing = std::find_if(
        plan.batches.begin(),
        plan.batches.end(),
        [instanceId](const RoutedBatch& batch)
        {
            return batch.instanceId == instanceId;
        });

    if (existing != plan.batches.end())
    {
        return *existing;
    }

    return plan.batches.emplace_back(
        RoutedBatch{
            .instanceId = instanceId
        });
}

} // namespace consolidator::core

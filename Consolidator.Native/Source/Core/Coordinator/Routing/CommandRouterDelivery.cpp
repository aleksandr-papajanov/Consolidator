#include "Core/Coordinator/Routing/CommandRouter.h"

#include <algorithm>
#include <cstddef>
#include <utility>

#include "Core/Coordinator/Delivery/CommandDeliveryQueue.h"
#include "Core/Coordinator/Routing/StateResponsePublisher.h"

namespace consolidator::core
{

void CommandRouter::PublishAndDeliver(
    const StateCommand& sourceCommand,
    WritePlan plan)
{
    const bool hasTopologyResponse =
        plan.topologyResponse.entries.size != 0;

    const std::size_t partCount =
        plan.batches.size() +
        (hasTopologyResponse ? 1U : 0U);

    const auto responseCount =
        static_cast<std::uint16_t>(
            std::max<std::size_t>(partCount, 1));

    StateResponsePublisher publisher{
        coordinatorResponses_,
        responseCount
    };

    std::uint16_t responseIndex = 0;

    if (hasTopologyResponse || plan.batches.empty())
    {
        publisher.Publish(
            std::move(plan.topologyResponse),
            responseIndex++);
    }

    for (auto& batch : plan.batches)
    {
        auto& message = batch.command.message;
        message.requestId = sourceCommand.message.requestId;
        message.responseInstanceId = sourceCommand.message.responseInstanceId;
        message.responseIndex = responseIndex++;
        message.responseCount = responseCount;
        deliveryQueue_.Enqueue(batch.instanceId, std::move(batch.command));
    }
}

} // namespace consolidator::core

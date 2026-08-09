#include "Core/Coordinator/Routing/CommandRouter.h"

#include <type_traits>
#include <utility>

#include "Core/Coordinator/Delivery/CommandDeliveryQueue.h"

namespace consolidator::core
{

void CommandRouter::Route(const InstanceCommand& command)
{
    std::visit(
        [this, sourceInstanceId = command.sourceInstanceId](
            const auto& typedCommand)
        {
            using CommandType =
                std::decay_t<decltype(typedCommand)>;

            if constexpr (
                std::is_same_v<CommandType, StateCommand>)
            {
                RouteStateCommand(
                    sourceInstanceId,
                    typedCommand);
            }
        },
        command.command);
}

void CommandRouter::RouteStateCommand(
    InstanceId sourceInstanceId,
    const StateCommand& command)
{
    if (!registry_.Contains(sourceInstanceId))
    {
        return;
    }

    if (command.operation == StateOperation::Read)
    {
        RouteRead(sourceInstanceId, command);
        return;
    }

    RouteWrite(sourceInstanceId, command);
}

void CommandRouter::RouteRead(
    InstanceId sourceInstanceId,
    const StateCommand& command)
{
    deliveryQueue_.Enqueue(
        sourceInstanceId,
        command);
}

void CommandRouter::RouteWrite(
    InstanceId sourceInstanceId,
    const StateCommand& command)
{
    auto plan =
        BuildWritePlan(sourceInstanceId, command);

    PublishAndDeliver(
        command,
        std::move(plan));
}

} // namespace consolidator::core

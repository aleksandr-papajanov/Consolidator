#include "Core/Coordinator/Routing/CommandRouter.h"

#include <cstddef>
#include <type_traits>
#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"

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
    auto* instance = registry_.FindInstance(sourceInstanceId);
    if (instance == nullptr)
    {
        return;
    }

    StateResponse response{
        command.message.requestId,
        command.message.responseInstanceId,
        sourceInstanceId,
        StateOperation::Read,
        {}};

    if (command.message.entries.size == 0)
    {
        instance->GetStateStore().ReadState(
            StatePath::Instance(sourceInstanceId),
            response.entries);
    }
    else
    {
        for (std::size_t index = 0; index < command.message.entries.size; ++index)
        {
            auto query = command.message.entries.entries[index].path;
            query.instanceId = sourceInstanceId;
            instance->GetStateStore().ReadState(query, response.entries);
        }
    }

    response.truncated = response.entries.truncated;
    for (std::size_t index = 0; index < response.entries.size; ++index)
    {
        constraintResolver_.Enrich(
            sourceInstanceId,
            response.entries.entries[index]);
    }
    coordinatorResponses_.Enqueue(std::move(response));
}

void CommandRouter::RouteWrite(
    InstanceId sourceInstanceId,
    const StateCommand& command)
{
    auto plan =
        BuildWritePlan(sourceInstanceId, command);

    PublishResponse(std::move(plan));
}

} // namespace consolidator::core

#include "Core/Routing/CommandRouter.h"

#include <cstddef>
#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

void CommandRouter::HandleReadCommand(
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
        for (std::size_t index = 0;
             index < command.message.entries.size;
             ++index)
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

} // namespace consolidator::core

#include "Core/Routing/CommandRouter.h"

#include <cstddef>
#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"
#include "Core/Routing/ParameterConstraintResolver.h"

namespace consolidator::core
{

CommandResult CommandRouter::Handle(const ReadStateCommand& command)
{
    if (!registry_.Contains(command.instanceId))
    {
        return NoCommandResponse{};
    }

    return HandleReadCommand(command);
}

StateResponse CommandRouter::HandleReadCommand(
    const ReadStateCommand& command)
{
    auto* instance = registry_.FindInstance(command.instanceId);
    if (instance == nullptr)
    {
        return StateResponse{command.requestId, command.instanceId, {}};
    }

    StateResponse response{
        command.requestId,
        command.instanceId,
        {}};

    if (command.queries.size == 0)
    {
        instance->GetStateStore().ReadState(
            StatePath::Instance(command.instanceId),
            response.entries);
    }
    else
    {
        for (std::size_t index = 0;
             index < command.queries.size;
             ++index)
        {
            auto query = command.queries.entries[index].path;
            query.instanceId = command.instanceId;
            instance->GetStateStore().ReadState(query, response.entries);
        }
    }

    response.truncated = response.entries.truncated;
    for (std::size_t index = 0; index < response.entries.size; ++index)
    {
        constraintResolver_.Enrich(
            command.instanceId,
            response.entries.entries[index]);
    }
    return response;
}

} // namespace consolidator::core

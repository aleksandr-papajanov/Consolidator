#include "Core/Instance/Handlers/StateCommandHandler.h"

#include <cstddef>
#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"
#include "Dsp/Processors/DspChain.h"

namespace consolidator::core
{

namespace
{

void QueueResponse(ConsolidatorInstance& instance, StateResponse response)
{
    instance.QueueStateResponse(std::move(response));
}

void ReadState(
    ConsolidatorInstance& instance,
    const StateCommand& command,
    StateResponse& response)
{
    if (command.message.entries.size == 0)
    {
        StatePath query;
        query.instanceId = instance.GetState().GetInstanceId();
        instance.GetState().ReadState(query, response.entries);
        instance.GetDspChain().ReadState(query, response.entries);
        return;
    }

    for (std::size_t index = 0; index < command.message.entries.size; ++index)
    {
        auto query = command.message.entries.entries[index].path;
        query.instanceId = instance.GetState().GetInstanceId();
        instance.GetState().ReadState(query, response.entries);
        instance.GetDspChain().ReadState(query, response.entries);
    }
}

void WriteState(
    ConsolidatorInstance& instance,
    const StateCommand& command,
    StateResponse& response)
{
    for (std::size_t index = 0; index < command.message.entries.size; ++index)
    {
        auto entry = command.message.entries.entries[index];
        entry.path.instanceId = instance.GetState().GetInstanceId();
        if (!instance.GetState().WriteState(entry, response.entries))
        {
            (void)instance.GetDspChain().WriteState(entry, response.entries);
        }
    }
}

} // namespace

void HandleStateCommand(
    ConsolidatorInstance& instance,
    const StateCommand& command)
{
    StateResponse response{
        command.message.requestId,
        command.message.responseInstanceId,
        command.operation,
        {}};
    if (command.operation == StateOperation::Read)
    {
        ReadState(instance, command, response);
    }
    else
    {
        WriteState(instance, command, response);
    }

    QueueResponse(instance, std::move(response));
}

} // namespace consolidator::core

#include "Core/Routing/CommandRouter.h"

#include <utility>

#include "Core/Routing/StateWriter.h"

namespace consolidator::core
{

void CommandRouter::HandleStateCommand(
    InstanceId sourceInstanceId,
    const StateCommand& command)
{
    if (!registry_.Contains(sourceInstanceId))
    {
        return;
    }

    if (command.operation == StateOperation::Read)
    {
        HandleReadCommand(sourceInstanceId, command);
        return;
    }

    auto response = stateWriter_.Write(sourceInstanceId, command);
    coordinatorResponses_.Enqueue(std::move(response));
}

} // namespace consolidator::core

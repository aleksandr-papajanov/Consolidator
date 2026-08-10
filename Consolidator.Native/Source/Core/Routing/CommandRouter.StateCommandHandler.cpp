#include "Core/Routing/CommandRouter.h"

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

    HandleWriteCommand(sourceInstanceId, command);
}

} // namespace consolidator::core

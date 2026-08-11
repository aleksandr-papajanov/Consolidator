#include "Core/Routing/CommandRouter.h"

#include "Core/Routing/StateWriter.h"

namespace consolidator::core
{

CommandResult CommandRouter::Handle(const WriteStateCommand& command)
{
    if (!registry_.Contains(command.instanceId))
    {
        return NoCommandResponse{};
    }

    return stateWriter_.Write(command);
}

} // namespace consolidator::core

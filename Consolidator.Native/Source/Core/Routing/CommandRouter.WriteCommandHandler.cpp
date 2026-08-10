#include "Core/Routing/CommandRouter.h"

#include "Core/Routing/StateWriter.h"

namespace consolidator::core
{

void CommandRouter::Handle(const WriteStateCommand& command)
{
    if (!registry_.Contains(command.instanceId))
    {
        return;
    }

    coordinatorResponses_.Enqueue(stateWriter_.Write(command));
}

} // namespace consolidator::core

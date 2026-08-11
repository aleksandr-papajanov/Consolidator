#include "Core/Routing/CommandRouter.h"

#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

CommandResult CommandRouter::Handle(const ResetDspCommand& command)
{
    if (!command.target.deviceId)
    {
        return NoCommandResponse{};
    }

    if (auto* instance = registry_.FindInstance(command.instanceId);
        instance != nullptr)
    {
        auto target = command.target;
        target.instanceId = command.instanceId;
        instance->EnqueueRealtimeCommand(target);
    }
    return NoCommandResponse{};
}

} // namespace consolidator::core

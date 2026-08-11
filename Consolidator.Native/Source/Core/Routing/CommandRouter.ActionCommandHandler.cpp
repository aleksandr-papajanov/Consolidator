#include "Core/Routing/CommandRouter.h"

#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

CommandResult CommandRouter::Handle(const ResetDspCommand& command)
{
    ActionResponse response{
        command.requestId,
        command.instanceId,
        ActionStatus::Rejected};
    if (!command.target.IsValidResetTarget())
    {
        return response;
    }

    auto* instance = registry_.FindInstance(command.instanceId);
    if (instance == nullptr)
    {
        return response;
    }

    auto target = command.target;
    target.instanceId = command.instanceId;
    if (instance->EnqueueRealtimeCommand(target))
    {
        response.status = ActionStatus::Accepted;
    }
    return response;
}

} // namespace consolidator::core

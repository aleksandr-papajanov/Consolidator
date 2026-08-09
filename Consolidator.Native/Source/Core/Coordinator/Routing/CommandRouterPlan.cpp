#include "Core/Coordinator/Routing/CommandRouter.h"

#include <cstddef>

namespace consolidator::core
{

CommandRouter::WritePlan CommandRouter::BuildWritePlan(
    InstanceId sourceInstanceId,
    const StateCommand& command)
{
    WritePlan plan{
        .topologyResponse = StateResponse{
            command.message.requestId,
            command.message.responseInstanceId,
            sourceInstanceId,
            command.operation,
            {}}
    };

    for (std::size_t index = 0;
         index < command.message.entries.size;
         ++index)
    {
        RouteWriteEntry(
            sourceInstanceId,
            command.message.entries.entries[index],
            plan);
    }

    return plan;
}

} // namespace consolidator::core

#include "Core/Instance/ConsolidatorInstance.h"

#include <exception>
#include <utility>

#include "Core/Coordinator/InstanceCoordinator.h"

namespace consolidator::core
{

void ConsolidatorInstance::EnqueueCommand(ReadStateCommand command)
{
    command.instanceId = GetInstanceId();
    InstanceCoordinator::Get().EnqueueCommand(Command{std::move(command)});
}

void ConsolidatorInstance::EnqueueCommand(WriteStateCommand command)
{
    command.instanceId = GetInstanceId();
    InstanceCoordinator::Get().EnqueueCommand(Command{std::move(command)});
}

void ConsolidatorInstance::EnqueueCommand(ResetDspCommand command)
{
    command.instanceId = GetInstanceId();
    InstanceCoordinator::Get().EnqueueCommand(Command{std::move(command)});
}

void ConsolidatorInstance::EnqueueParameterUpdates(
    std::span<const ParameterUpdate> updates)
{
    runtimeUpdateMailbox_.EnqueueParameters(updates);
}

void ConsolidatorInstance::EnqueueRuntimeUpdates(
    std::span<const RuntimeControlUpdate> updates)
{
    runtimeUpdateMailbox_.EnqueueRuntimeControls(updates);
}

void ConsolidatorInstance::EnqueueRealtimeCommand(const StatePath& target)
{
    if (!realtimeCommandQueue_.TryEnqueue(
            RealtimeCommand{ResetRuntimeCommand{target}}))
    {
        std::terminate();
    }
}

} // namespace consolidator::core

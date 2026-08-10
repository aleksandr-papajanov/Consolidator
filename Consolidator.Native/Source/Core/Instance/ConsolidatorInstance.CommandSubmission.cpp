#include "Core/Instance/ConsolidatorInstance.h"

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

} // namespace consolidator::core

#include "Core/Coordinator/InstanceCoordinator.h"

#include <utility>

namespace consolidator::core
{

void InstanceCoordinator::EnqueueStateCommand(
    InstanceId sourceInstanceId,
    StateCommand command)
{
    commandQueue_.Enqueue(InstanceCommand{
        sourceInstanceId,
        Command{std::move(command)}});
    wakeCondition_.notify_one();
}

} // namespace consolidator::core

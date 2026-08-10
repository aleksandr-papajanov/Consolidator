#include "Core/Coordinator/InstanceCoordinator.h"

#include <utility>

namespace consolidator::core
{

void InstanceCoordinator::EnqueueCommand(Command command)
{
    commandQueue_.Enqueue(std::move(command));
    wakeCondition_.notify_one();
}

} // namespace consolidator::core

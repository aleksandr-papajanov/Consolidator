#include "Core/Coordinator/InstanceCoordinator.h"

#include <chrono>

namespace consolidator::core
{

void InstanceCoordinator::WorkerLoop(std::stop_token stopToken)
{
    while (!stopToken.stop_requested())
    {
        std::unique_lock wakeLock{wakeMutex_};
        wakeCondition_.wait_for(wakeLock, std::chrono::milliseconds{1}, [this]
        {
            return commandQueue_.HasCommands();
        });
        wakeLock.unlock();

        std::lock_guard registryLock{registryMutex_};
        while (const auto command = commandQueue_.TryDequeue())
        {
            commandRouter_.HandleCommand(*command);
        }
    }
}

} // namespace consolidator::core

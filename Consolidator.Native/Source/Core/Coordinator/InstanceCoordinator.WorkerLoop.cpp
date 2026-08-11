#include "Core/Coordinator/InstanceCoordinator.h"

#include <chrono>
#include <type_traits>
#include <utility>

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
            const auto result = commandRouter_.HandleCommand(*command);
            std::visit(
                [this](auto&& typedResult)
                {
                    using ResultType = std::decay_t<decltype(typedResult)>;
                    if constexpr (std::is_same_v<ResultType, StateWriteResult>)
                    {
                        if (typedResult.effects.audibilityChanged)
                        {
                            RefreshAudibility();
                        }
                        coordinatorResponses_.Enqueue(
                            std::move(typedResult.response));
                    }
                    else if constexpr (std::is_same_v<ResultType, StateResponse>)
                    {
                        coordinatorResponses_.Enqueue(std::move(typedResult));
                    }
                },
                result);
        }
    }
}

} // namespace consolidator::core

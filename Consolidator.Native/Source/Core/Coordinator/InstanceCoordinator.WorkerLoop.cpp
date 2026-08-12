#include "Core/Coordinator/InstanceCoordinator.h"

#include <chrono>
#include <type_traits>
#include <utility>
#include <vector>

#include "Core/Instance/ConsolidatorInstance.h"

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

        {
            std::vector<ConsolidatorInstance::ResponseNotifierHandle> notifiers;
            {
                std::lock_guard registryLock{registryMutex_};
                for (auto* instance : registry_.GetInstances())
                {
                    instance->RefreshEqualizerResponseRequest();
                }
                while (const auto command = commandQueue_.TryDequeue())
                {
                    const auto result = commandRouter_.HandleCommand(*command);
                    std::visit(
                        [this, &notifiers](auto&& typedResult)
                        {
                            using ResultType = std::decay_t<decltype(typedResult)>;
                            if constexpr (std::is_same_v<ResultType, StateWriteResult>)
                            {
                                if (typedResult.effects.audibilityChanged)
                                {
                                    RefreshAudibility();
                                }
                                const auto instanceId = typedResult.response.instanceId;
                                if (auto* instance = registry_.FindInstance(instanceId))
                                {
                                    instance->EnqueueResponse(
                                        CommandResponse{std::move(typedResult.response)});
                                    if (auto notifier = instance->GetResponseNotifierHandle())
                                    {
                                        notifiers.push_back(std::move(notifier));
                                    }
                                }
                            }
                            else if constexpr (std::is_same_v<ResultType, StateResponse> ||
                                               std::is_same_v<ResultType, ActionResponse>)
                            {
                                const auto instanceId = typedResult.instanceId;
                                if (auto* instance = registry_.FindInstance(instanceId))
                                {
                                    instance->EnqueueResponse(
                                        CommandResponse{std::move(typedResult)});
                                    if (auto notifier = instance->GetResponseNotifierHandle())
                                    {
                                        notifiers.push_back(std::move(notifier));
                                    }
                                }
                            }
                        },
                        result);
                }
            }
            for (auto& notifier : notifiers)
            {
                ConsolidatorInstance::NotifyResponseAvailable(
                    std::move(notifier));
            }
        }
    }
}

} // namespace consolidator::core

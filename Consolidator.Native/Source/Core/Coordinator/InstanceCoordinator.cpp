#include "Core/Coordinator/InstanceCoordinator.h"

#include <chrono>
#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

InstanceCoordinator& InstanceCoordinator::Get()
{
    static InstanceCoordinator instance;
    return instance;
}

InstanceCoordinator::InstanceCoordinator()
    : stateRouter_(registry_)
    , constraintResolver_(registry_, stateRouter_)
    , commandRouter_(registry_, stateRouter_, constraintResolver_, coordinatorResponses_)
    , worker_([this](std::stop_token stopToken)
      {
          WorkerLoop(stopToken);
      })
{
}

InstanceCoordinator::~InstanceCoordinator()
{
    worker_.request_stop();
    wakeCondition_.notify_all();
}

void InstanceCoordinator::RegisterInstance(ConsolidatorInstance& instance)
{
    {
        std::lock_guard lock{registryMutex_};
        const auto instanceId = nextInstanceId_;
        nextInstanceId_ = InstanceId{nextInstanceId_.GetValue() + 1};
        instance.state_.SetInstanceId(instanceId);
        registry_.RegisterInstance(instanceId, &instance);
    }
}

void InstanceCoordinator::UnregisterInstance(InstanceId instanceId)
{
    std::lock_guard lock{registryMutex_};
    if (const auto* instance = registry_.FindInstance(instanceId))
    {
        registry_.UnregisterInstance(instanceId, instance->state_);
    }
}

void InstanceCoordinator::EnqueueCommand(InstanceId sourceInstanceId, Command command)
{
    commandQueue_.Enqueue(InstanceCommand{sourceInstanceId, std::move(command)});
    wakeCondition_.notify_one();
}

std::optional<StateResponse> InstanceCoordinator::TryDequeueResponse()
{
    return coordinatorResponses_.TryDequeue();
}

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
            commandRouter_.Route(*command);
        }
    }
}

} // namespace consolidator::core

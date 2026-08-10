#include "Core/Coordinator/InstanceCoordinator.h"

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
    , stateWriter_(
          registry_,
          stateRouter_,
          constraintResolver_)
    , commandRouter_(
          registry_,
          constraintResolver_,
          stateWriter_,
          coordinatorResponses_)
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

std::optional<StateResponse> InstanceCoordinator::TryDequeueResponse()
{
    return coordinatorResponses_.TryDequeue();
}

} // namespace consolidator::core

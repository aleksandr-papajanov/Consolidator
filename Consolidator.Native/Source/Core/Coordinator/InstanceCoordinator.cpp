#include "Core/Coordinator/InstanceCoordinator.h"

#include "Core/Instance/ConsolidatorInstance.h"

#include <span>
#include <unordered_map>

namespace consolidator::core
{

InstanceCoordinator& InstanceCoordinator::Get()
{
    static InstanceCoordinator instance;
    return instance;
}

InstanceCoordinator::InstanceCoordinator()
    : groupGraph_(registry_)
    , stateRouter_(registry_, groupGraph_)
    , constraintResolver_(registry_, stateRouter_)
    , instanceAudibilityResolver_(registry_, groupGraph_)
    , stateWriter_(
          registry_,
          stateRouter_,
          constraintResolver_)
    , commandRouter_(
          registry_,
          constraintResolver_,
          stateWriter_)
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
        instance.stateStore_.SetInstanceId(instanceId);
        registry_.RegisterInstance(instanceId, &instance);
    }
}

void InstanceCoordinator::UnregisterInstance(InstanceId instanceId)
{
    std::lock_guard lock{registryMutex_};
    if (const auto* instance = registry_.FindInstance(instanceId))
    {
        registry_.UnregisterInstance(instanceId, instance->GetStateStore().GetInstance());
    }
}

void InstanceCoordinator::RefreshAudibility()
{
    std::vector<RuntimeControlUpdate> updates;
    instanceAudibilityResolver_.Resolve(updates);

    std::unordered_map<InstanceId, std::vector<RuntimeControlUpdate>> updatesByInstance;
    for (const auto& update : updates)
    {
        if (!update.target.instanceId)
        {
            continue;
        }
        updatesByInstance[*update.target.instanceId].push_back(update);
    }

    for (auto& [instanceId, instanceUpdates] : updatesByInstance)
    {
        auto* instance = registry_.FindInstance(instanceId);
        if (instance == nullptr)
        {
            continue;
        }
        instance->EnqueueRuntimeUpdates(
            std::span<const RuntimeControlUpdate>{
                instanceUpdates.data(),
                instanceUpdates.size()});
    }
}

} // namespace consolidator::core

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
          registryState_,
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
    std::vector<ConsolidatorInstance::RegistryNotifierHandle> notifiers;
    std::uint64_t revision = 0;
    {
        std::lock_guard lock{registryMutex_};
        const auto instanceId = nextInstanceId_;
        nextInstanceId_ = InstanceId{nextInstanceId_.GetValue() + 1};
        instance.stateStore_.SetInstanceId(instanceId);
        registry_.RegisterInstance(instanceId, &instance);
        if (registryState_.Refresh(registry_))
        {
            revision = registryState_.Get().revision;
            for (auto* registered : registry_.GetInstances())
            {
                notifiers.push_back(registered->GetRegistryNotifierHandle());
            }
        }
    }
    PublishRegistryChanged(std::move(notifiers), revision);
}

void InstanceCoordinator::UnregisterInstance(InstanceId instanceId)
{
    {
        std::vector<ConsolidatorInstance::RegistryNotifierHandle> notifiers;
        std::uint64_t revision = 0;
        {
            std::lock_guard lock{registryMutex_};
            if (const auto* instance = registry_.FindInstance(instanceId))
            {
                registry_.UnregisterInstance(
                    instanceId,
                    instance->GetStateStore().GetInstance());
                if (registryState_.Refresh(registry_))
                {
                    revision = registryState_.Get().revision;
                    for (auto* registered : registry_.GetInstances())
                    {
                        notifiers.push_back(
                            registered->GetRegistryNotifierHandle());
                    }
                }
            }
        }
        PublishRegistryChanged(std::move(notifiers), revision);
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

void InstanceCoordinator::PublishRegistryChanged(
    std::vector<ConsolidatorInstance::RegistryNotifierHandle> notifiers,
    std::uint64_t revision)
{
    if (revision == 0)
    {
        return;
    }
    for (auto& notifier : notifiers)
    {
        ConsolidatorInstance::NotifyRegistryChanged(
            std::move(notifier), revision);
    }
}

} // namespace consolidator::core

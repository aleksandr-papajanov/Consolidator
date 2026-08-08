#include "Core/Coordinator/InstanceCoordinator.h"

#include <cstdint>
#include <chrono>
#include <type_traits>
#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

namespace
{

dsp::RouteNodeId ToRouteNodeId(dsp::BankId bankId) noexcept
{
    return static_cast<dsp::RouteNodeId>(
        static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) +
        dsp::detail::ToIndex(bankId));
}

DspParameterChangeCommand CreateCommandForBank(
    const DspParameterChangeCommand& command,
    dsp::BankId bankId)
{
    const auto& route = command.route;
    const auto bankNodeId = ToRouteNodeId(bankId);

    if (route.GetDepth() == 1)
    {
        return {dsp::ParameterRoute{route.GetDeviceId(), route.GetParameterId(), bankNodeId}, command.value};
    }

    if (route.GetDepth() == 2)
    {
        return {dsp::ParameterRoute{route.GetDeviceId(), route.GetParameterId(), bankNodeId, route.GetNode(1)}, command.value};
    }

    return {dsp::ParameterRoute{route.GetDeviceId(), route.GetParameterId(), bankNodeId, route.GetNode(1), route.GetNode(2)}, command.value};
}

} // namespace

InstanceCoordinator& InstanceCoordinator::Get()
{
    static InstanceCoordinator instance;
    return instance;
}

InstanceCoordinator::InstanceCoordinator()
    : worker_([this](std::stop_token stopToken)
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
    std::lock_guard lock{registryMutex_};
    const auto instanceId = nextInstanceId_;
    nextInstanceId_ = InstanceId{nextInstanceId_.GetValue() + 1};
    instance.state_.SetInstanceId(instanceId);
    registry_.RegisterInstance(instanceId, &instance);
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
        RetryPendingDeliveries();
        while (const auto command = commandQueue_.TryDequeue())
        {
            RouteCommand(*command);
        }
    }
}

void InstanceCoordinator::RouteCommand(const InstanceCommand& command)
{
    std::visit(
        [this, sourceInstanceId = command.sourceInstanceId](const auto& typedCommand)
        {
            using CommandType = std::decay_t<decltype(typedCommand)>;
            if constexpr (std::is_same_v<CommandType, DspParameterChangeCommand>)
            {
                RouteDspParameterChangeCommand(sourceInstanceId, typedCommand);
            }
        },
        command.command);
}

void InstanceCoordinator::RouteDspParameterChangeCommand(
    InstanceId sourceInstanceId,
    const DspParameterChangeCommand& command)
{
    auto* const source = registry_.FindInstance(sourceInstanceId);
    if (source == nullptr)
    {
        return;
    }

    if (command.route.GetDeviceId() != dsp::DeviceId::Equalizer)
    {
        EnqueueForInstance(sourceInstanceId, command);
        return;
    }

    const auto sourceBankId = source->state_.GetSelectedBankId();
    const auto groupId = source->state_.GetBankState(sourceBankId).GetGroupId();
    if (!groupId)
    {
        EnqueueForInstance(sourceInstanceId, CreateCommandForBank(command, sourceBankId));
        return;
    }

    for (const auto target : registry_.FindGroupMembers(*groupId))
    {
        EnqueueForInstance(target.instanceId, CreateCommandForBank(command, target.bankId));
    }
}

void InstanceCoordinator::EnqueueForInstance(InstanceId instanceId, Command command)
{
    auto* const instance = registry_.FindInstance(instanceId);
    if (instance == nullptr)
    {
        return;
    }

    if (!instance->EnqueueLocalCommand(command))
    {
        instance->RecordLocalQueueOverflow();
        pendingDeliveries_.push_back(PendingLocalCommand{instanceId, std::move(command)});
    }
}

void InstanceCoordinator::RetryPendingDeliveries()
{
    auto pendingIt = pendingDeliveries_.begin();
    while (pendingIt != pendingDeliveries_.end())
    {
        auto* const instance = registry_.FindInstance(pendingIt->instanceId);
        if (instance == nullptr || instance->EnqueueLocalCommand(pendingIt->command))
        {
            pendingIt = pendingDeliveries_.erase(pendingIt);
        }
        else
        {
            instance->RecordLocalQueueOverflow();
            ++pendingIt;
        }
    }
}

} // namespace consolidator::core

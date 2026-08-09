#include "Core/Coordinator/InstanceCoordinator.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
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

StateEntry CreateEntryForBank(
    const StateEntry& entry,
    dsp::BankId bankId)
{
    auto result = entry;
    if (!result.path.deviceId || !result.path.parameterId)
    {
        return result;
    }

    const core::StatePath route{
        *result.path.deviceId,
        *result.path.parameterId,
        result.path.depth > 0 ? result.path.nodes[0] : dsp::RouteNodeId::Bank0};
    result.path.deviceId = route.GetDeviceId();
    result.path.parameterId = route.GetParameterId();
    result.path.depth = route.GetDepth();
    result.path.nodes[0] = ToRouteNodeId(bankId);
    if (result.path.depth > 1)
    {
        result.path.nodes[1] = route.GetNode(1);
    }
    if (result.path.depth > 2)
    {
        result.path.nodes[2] = route.GetNode(2);
    }
    return result;
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

std::optional<StateResponse> InstanceCoordinator::TryDequeueResponse(InstanceId instanceId)
{
    std::lock_guard lock{registryMutex_};
    auto pendingIt = std::find_if(
        pendingResponses_.begin(),
        pendingResponses_.end(),
        [instanceId](const StateResponse& response)
        {
            return response.responseInstanceId == instanceId;
        });
    if (pendingIt != pendingResponses_.end())
    {
        auto response = std::move(*pendingIt);
        pendingResponses_.erase(pendingIt);
        return response;
    }

    for (auto* instance : registry_.GetInstances())
    {
        while (const auto response = instance->TryDequeueResponse())
        {
            if (response->responseInstanceId == instanceId)
            {
                return response;
            }

            pendingResponses_.push_back(*response);
        }
    }

    return std::nullopt;
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
            if constexpr (std::is_same_v<CommandType, StateCommand>)
            {
                RouteStateCommand(sourceInstanceId, typedCommand);
            }
        },
        command.command);
}


void InstanceCoordinator::RouteStateCommand(
    InstanceId sourceInstanceId,
    const StateCommand& command)
{
    auto* const source = registry_.FindInstance(sourceInstanceId);
    if (source == nullptr)
    {
        return;
    }

    if (command.operation == StateOperation::Read || command.message.entries.size != 1)
    {
        EnqueueForInstance(sourceInstanceId, command);
        return;
    }

    const auto& entry = command.message.entries.entries[0];
    const auto targets = ResolveWriteTargets(sourceInstanceId, entry.path);
    if (targets.empty())
    {
        EnqueueForInstance(sourceInstanceId, command);
        return;
    }

    for (const auto target : targets)
    {
        auto routed = command;
        routed.message.entries.entries[0] = CreateEntryForBank(entry, target.bankId);
        EnqueueForInstance(target.instanceId, std::move(routed));
    }
}

std::vector<BankAddress> InstanceCoordinator::ResolveWriteTargets(
    InstanceId sourceInstanceId,
    const StatePath& path) const
{
    if (!path.deviceId || *path.deviceId != dsp::DeviceId::Equalizer)
    {
        return {};
    }

    const auto* source = registry_.FindInstance(sourceInstanceId);
    if (source == nullptr)
    {
        return {};
    }

    const auto sourceBankId = source->state_.GetSelectedBankId();
    const auto groupId = source->state_.GetBankState(sourceBankId).GetGroupId();
    if (!groupId)
    {
        return {BankAddress{sourceInstanceId, sourceBankId}};
    }

    const auto members = registry_.FindGroupMembers(*groupId);
    return {members.begin(), members.end()};
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

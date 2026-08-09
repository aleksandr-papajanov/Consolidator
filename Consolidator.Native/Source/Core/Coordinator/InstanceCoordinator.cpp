#include "Core/Coordinator/InstanceCoordinator.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"
#include "Core/Coordinator/StateResponseCollector.h"

namespace consolidator::core
{

InstanceCoordinator& InstanceCoordinator::Get()
{
    static InstanceCoordinator instance;
    return instance;
}

InstanceCoordinator::InstanceCoordinator()
    : stateRouter_(registry_)
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
        RetryPendingDeliveries();
        while (const auto command = commandQueue_.TryDequeue())
        {
            RouteCommand(*command);
        }
        DrainInstanceResponses();
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

    if (command.operation == StateOperation::Read)
    {
        EnqueueForInstance(sourceInstanceId, command);
        return;
    }

    StateResponse topologyResponse{
        command.message.requestId,
        command.message.responseInstanceId,
        sourceInstanceId,
        command.operation,
        {}};
    struct RoutedBatch
    {
        InstanceId instanceId;
        StateCommand command;
    };
    std::vector<RoutedBatch> routedBatches;
    const auto getBatch = [&routedBatches](InstanceId instanceId) -> RoutedBatch&
    {
        auto batchIt = std::find_if(routedBatches.begin(), routedBatches.end(),
            [instanceId](const RoutedBatch& batch) { return batch.instanceId == instanceId; });
        if (batchIt == routedBatches.end())
        {
            batchIt = routedBatches.emplace(
                routedBatches.end(), instanceId, StateCommand{StateOperation::Write, {}});
        }
        return *batchIt;
    };

    for (std::size_t index = 0; index < command.message.entries.size; ++index)
    {
        const auto& entry = command.message.entries.entries[index];
        if (ApplyTopologyWrite(sourceInstanceId, entry, topologyResponse.entries))
        {
            continue;
        }

        const auto targets = stateRouter_.ResolveTargets(sourceInstanceId, entry.path);
        if (targets.empty())
        {
            (void)getBatch(sourceInstanceId).command.message.entries.TryAppend(entry);
            continue;
        }

        for (const auto target : targets)
        {
            (void)getBatch(target.instanceId).command.message.entries.TryAppend(
                StateRouter::ForBank(entry, target.bankId));
        }
    }

    const bool hasTopologyResponse = topologyResponse.entries.size != 0;
    const auto responseCount = static_cast<std::uint16_t>(routedBatches.size() +
        (hasTopologyResponse ? 1 : 0));
    StateResponseCollector responses{
        coordinatorResponses_,
        static_cast<std::uint16_t>(responseCount == 0 ? 1 : responseCount)};
    if (hasTopologyResponse)
    {
        responses.Publish(std::move(topologyResponse), 0);
    }
    if (!hasTopologyResponse && routedBatches.empty())
    {
        responses.Publish(std::move(topologyResponse), 0);
    }
    for (std::size_t index = 0; index < routedBatches.size(); ++index)
    {
        auto& batch = routedBatches[index];
        batch.command.message.requestId = command.message.requestId;
        batch.command.message.responseInstanceId = command.message.responseInstanceId;
        batch.command.message.responseIndex = static_cast<std::uint16_t>(index +
            (hasTopologyResponse ? 1 : 0));
        batch.command.message.responseCount = responseCount;
        EnqueueForInstance(batch.instanceId, std::move(batch.command));
    }
}

void InstanceCoordinator::DrainInstanceResponses()
{
    for (auto* instance : registry_.GetInstances())
    {
        while (const auto response = instance->TryDequeueResponse())
        {
            coordinatorResponses_.Enqueue(std::move(*response));
        }
    }
}

bool InstanceCoordinator::ApplyTopologyWrite(
    InstanceId sourceInstanceId,
    const StateEntry& entry,
    StateResponseEntries& applied)
{
    auto* const source = registry_.FindInstance(sourceInstanceId);
    if (source == nullptr || !entry.path.field)
    {
        return false;
    }

    std::optional<BankAddress> changedBank;
    std::optional<GroupId> previousGroup;
    if (*entry.path.field == StateField::GroupId && entry.path.depth > 0)
    {
        const auto bankNode = static_cast<std::uint8_t>(entry.path.nodes[0]);
        const auto firstBankNode = static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0);
        const auto lastBankNode = firstBankNode + InstanceState::kBankCount - 1;
        if (bankNode < firstBankNode || bankNode > lastBankNode)
        {
            return false;
        }
        const auto bankId = static_cast<dsp::BankId>(bankNode - firstBankNode);
        changedBank = BankAddress{sourceInstanceId, bankId};
        previousGroup = source->state_.GetBankState(bankId).GetGroupId();
    }

    const auto status = source->state_.WriteState(entry, applied);
    if (status == StateWriteStatus::NotHandled)
    {
        return false;
    }
    if (changedBank)
    {
        registry_.CacheBankGroup(
            *changedBank,
            previousGroup,
            source->state_.GetBankState(changedBank->bankId).GetGroupId());
    }
    return true;
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

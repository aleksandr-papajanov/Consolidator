#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "Core/Commands/CommandQueue.h"
#include "Core/Commands/Commands.h"
#include "Core/Registry/InstanceRegistry.h"
#include "Core/State/StateProtocol.h"
#include "Core/Notifications/Notifications.h"
#include "Core/State/BankState.h"
#include <optional>

namespace consolidator::core
{

class ConsolidatorInstance;

class InstanceCoordinator
{
public:
    static InstanceCoordinator& Get();

    void EnqueueCommand(InstanceId sourceInstanceId, Command command);
    [[nodiscard]] std::optional<StateResponse> TryDequeueResponse(InstanceId instanceId);

    ~InstanceCoordinator();

    InstanceCoordinator(const InstanceCoordinator&) = delete;
    InstanceCoordinator& operator=(const InstanceCoordinator&) = delete;

private:
    friend class ConsolidatorInstance;

    InstanceCoordinator();

    void RegisterInstance(ConsolidatorInstance& instance);
    void UnregisterInstance(InstanceId instanceId);
    void WorkerLoop(std::stop_token stopToken);
    void RouteCommand(const InstanceCommand& command);
    void RouteStateCommand(
        InstanceId sourceInstanceId,
        const StateCommand& command);
    [[nodiscard]] std::vector<BankAddress> ResolveWriteTargets(
        InstanceId sourceInstanceId,
        const StatePath& path) const;

    void EnqueueForInstance(InstanceId instanceId, Command command);
    void RetryPendingDeliveries();

    struct PendingLocalCommand
    {
        InstanceId instanceId;
        Command command;
    };

    InstanceId nextInstanceId_{0};
    InstanceRegistry registry_;
    CommandQueue<InstanceCommand> commandQueue_;
    std::vector<PendingLocalCommand> pendingDeliveries_;
    std::vector<StateResponse> pendingResponses_;
    mutable std::mutex registryMutex_;
    std::mutex wakeMutex_;
    std::condition_variable_any wakeCondition_;
    std::jthread worker_;
};

} // namespace consolidator::core

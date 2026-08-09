#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <thread>

#include "Core/Commands/ConcurrentQueue.h"
#include "Core/Commands/Commands.h"
#include "Core/Coordinator/Routing/CommandRouter.h"
#include "Core/Coordinator/Routing/StateRouter.h"
#include "Core/Coordinator/Routing/ParameterConstraintResolver.h"
#include "Core/Notifications/Notifications.h"
#include "Core/Registry/InstanceRegistry.h"

namespace consolidator::core
{

class ConsolidatorInstance;

class InstanceCoordinator
{
public:
    static InstanceCoordinator& Get();

    void EnqueueCommand(InstanceId sourceInstanceId, Command command);
    [[nodiscard]] std::optional<StateResponse> TryDequeueResponse();

    ~InstanceCoordinator();

    InstanceCoordinator(const InstanceCoordinator&) = delete;
    InstanceCoordinator& operator=(const InstanceCoordinator&) = delete;

private:
    friend class ConsolidatorInstance;

    InstanceCoordinator();

    void RegisterInstance(ConsolidatorInstance& instance);
    void UnregisterInstance(InstanceId instanceId);
    void WorkerLoop(std::stop_token stopToken);

    InstanceId nextInstanceId_{0};
    InstanceRegistry registry_;
    StateRouter stateRouter_;
    ParameterConstraintResolver constraintResolver_;
    ConcurrentQueue<StateResponse> coordinatorResponses_;
    CommandRouter commandRouter_;
    ConcurrentQueue<InstanceCommand> commandQueue_;
    mutable std::mutex registryMutex_;
    std::mutex wakeMutex_;
    std::condition_variable_any wakeCondition_;
    std::jthread worker_;
};

} // namespace consolidator::core

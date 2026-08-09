#pragma once

#include <cstdint>
#include <vector>

#include "Core/Commands/ConcurrentQueue.h"
#include "Core/Commands/Commands.h"
#include "Core/Instance/Queues/DspUpdateMailbox.h"
#include "Core/Coordinator/Routing/StateRouter.h"
#include "Core/Coordinator/Routing/ParameterConstraintResolver.h"
#include "Core/Notifications/Notifications.h"
#include "Core/Registry/InstanceRegistry.h"

namespace consolidator::core
{

class CommandRouter
{
public:
    CommandRouter(
        InstanceRegistry& registry,
        const StateRouter& stateRouter,
        const ParameterConstraintResolver& constraintResolver,
        ConcurrentQueue<StateResponse>& coordinatorResponses) noexcept;

    void Route(const InstanceCommand& command);

private:
    struct WritePlan
    {
        struct PendingDspUpdate
        {
            InstanceId instanceId;
            DspUpdate update;
        };

        StateResponse coordinatorResponse;
        std::vector<StatePath> affectedConstraintPaths;
        std::vector<PendingDspUpdate> pendingDspUpdates;
    };

    void RouteStateCommand(
        InstanceId sourceInstanceId,
        const StateCommand& command);

    void RouteRead(
        InstanceId sourceInstanceId,
        const StateCommand& command);

    void RouteWrite(
        InstanceId sourceInstanceId,
        const StateCommand& command);

    [[nodiscard]] WritePlan BuildWritePlan(
        InstanceId sourceInstanceId,
        const StateCommand& command);

    void RouteWriteEntry(
        InstanceId sourceInstanceId,
        const StateEntry& entry,
        WritePlan& plan);

    [[nodiscard]] bool ApplyStateStoreWrite(
        InstanceId targetInstanceId,
        const StateEntry& entry,
        WritePlan& plan);

    void CollectConstraintDependencyPaths(
        InstanceId targetInstanceId,
        const StateEntry& entry,
        WritePlan& plan);

    void PublishResponse(WritePlan plan);

    void PublishDspUpdates(WritePlan& plan);
    void RefreshConstraintEntries(WritePlan& plan);
    void PublishStateResponse(WritePlan plan);

    [[nodiscard]] bool ApplyTopologyWrite(
        InstanceId sourceInstanceId,
        const StateEntry& entry,
        StateResponseEntries& applied,
        std::vector<BankAddress>& affectedBanks);

    InstanceRegistry& registry_;
    const StateRouter& stateRouter_;
    const ParameterConstraintResolver& constraintResolver_;
    ConcurrentQueue<StateResponse>& coordinatorResponses_;
};

} // namespace consolidator::core

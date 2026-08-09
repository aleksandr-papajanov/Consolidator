#pragma once

#include <vector>

#include "Core/Commands/ConcurrentQueue.h"
#include "Core/Commands/Commands.h"
#include "Core/Coordinator/Routing/StateRouter.h"
#include "Core/Notifications/Notifications.h"
#include "Core/Registry/InstanceRegistry.h"

namespace consolidator::core
{

class CommandDeliveryQueue;

class CommandRouter
{
public:
    CommandRouter(
        InstanceRegistry& registry,
        const StateRouter& stateRouter,
        CommandDeliveryQueue& deliveryQueue,
        ConcurrentQueue<StateResponse>& coordinatorResponses) noexcept;

    void Route(const InstanceCommand& command);

private:
    struct RoutedBatch
    {
        InstanceId instanceId;
        StateCommand command{StateOperation::Write, {}};
    };

    struct WritePlan
    {
        StateResponse topologyResponse;
        std::vector<RoutedBatch> batches;
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

    [[nodiscard]] RoutedBatch& GetOrCreateBatch(
        WritePlan& plan,
        InstanceId instanceId);

    void PublishAndDeliver(
        const StateCommand& sourceCommand,
        WritePlan plan);

    [[nodiscard]] bool ApplyTopologyWrite(
        InstanceId sourceInstanceId,
        const StateEntry& entry,
        StateResponseEntries& applied);

    InstanceRegistry& registry_;
    const StateRouter& stateRouter_;
    CommandDeliveryQueue& deliveryQueue_;
    ConcurrentQueue<StateResponse>& coordinatorResponses_;
};

} // namespace consolidator::core

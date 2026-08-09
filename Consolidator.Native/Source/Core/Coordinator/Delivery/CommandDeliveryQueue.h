#pragma once

#include <deque>
#include <unordered_map>

#include "Core/Commands/Commands.h"
#include "Core/Registry/InstanceRegistry.h"

namespace consolidator::core
{

class CommandDeliveryQueue
{
public:
    explicit CommandDeliveryQueue(InstanceRegistry& registry) noexcept;

    void Enqueue(InstanceId instanceId, Command command);
    void RetryPending();
    void RemoveForInstance(InstanceId instanceId);

private:
    InstanceRegistry& registry_;
    std::unordered_map<InstanceId, std::deque<Command>> pendingByInstance_;
};

} // namespace consolidator::core

#include "Core/Coordinator/Delivery/CommandDeliveryQueue.h"

#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

CommandDeliveryQueue::CommandDeliveryQueue(InstanceRegistry& registry) noexcept
    : registry_(registry)
{
}

void CommandDeliveryQueue::Enqueue(InstanceId instanceId, Command command)
{
    auto* const instance = registry_.FindInstance(instanceId);
    if (instance == nullptr)
    {
        return;
    }

    const auto pendingIt = pendingByInstance_.find(instanceId);
    if (pendingIt != pendingByInstance_.end())
    {
        pendingIt->second.push_back(std::move(command));
        return;
    }

    if (!instance->EnqueueLocalCommand(command))
    {
        instance->RecordLocalQueueOverflow();
        pendingByInstance_[instanceId].push_back(std::move(command));
    }
}

void CommandDeliveryQueue::RetryPending()
{
    for (auto pendingIt = pendingByInstance_.begin();
         pendingIt != pendingByInstance_.end();)
    {
        auto* const instance = registry_.FindInstance(pendingIt->first);
        if (instance == nullptr)
        {
            pendingIt = pendingByInstance_.erase(pendingIt);
            continue;
        }

        auto& commands = pendingIt->second;
        while (!commands.empty())
        {
            if (!instance->EnqueueLocalCommand(commands.front()))
            {
                break;
            }

            commands.pop_front();
        }

        if (commands.empty())
        {
            pendingIt = pendingByInstance_.erase(pendingIt);
        }
        else
        {
            ++pendingIt;
        }
    }
}

void CommandDeliveryQueue::RemoveForInstance(InstanceId instanceId)
{
    pendingByInstance_.erase(instanceId);
}

} // namespace consolidator::core

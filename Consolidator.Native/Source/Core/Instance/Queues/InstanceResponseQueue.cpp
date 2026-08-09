#include "Core/Instance/Queues/InstanceResponseQueue.h"

#include <utility>

namespace consolidator::core
{

bool InstanceResponseQueue::CanEnqueue() const noexcept
{
    return queue_.CanEnqueue();
}

bool InstanceResponseQueue::Enqueue(StateResponse response) noexcept
{
    return queue_.TryEnqueue(std::move(response));
}

std::optional<StateResponse> InstanceResponseQueue::TryDequeue()
{
    return queue_.TryDequeue();
}

} // namespace consolidator::core

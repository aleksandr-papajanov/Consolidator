#include "Core/Instance/Queues/InstanceCommandQueue.h"

#include <cassert>
#include <type_traits>
#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"
#include "Core/Instance/Handlers/StateCommandHandler.h"
#include "Core/Instance/Queues/InstanceResponseQueue.h"

namespace consolidator::core
{

bool InstanceCommandQueue::TryEnqueue(Command command)
{
    return queue_.TryEnqueue(std::move(command));
}

void InstanceCommandQueue::Process(
    ConsolidatorInstance& instance,
    InstanceResponseQueue& responseQueue)
{
    // Every current local command produces exactly one response. Do not dequeue
    // a command until the response queue can accept that response.
    while (responseQueue.CanEnqueue())
    {
        const auto command = queue_.TryDequeue();
        if (!command)
        {
            return;
        }

        std::visit(
            [&instance, &responseQueue](const auto& typedCommand)
            {
                using CommandType = std::decay_t<decltype(typedCommand)>;
                if constexpr (std::is_same_v<CommandType, StateCommand>)
                {
                    auto response = HandleStateCommand(instance, typedCommand);
                    const bool queued =
                        responseQueue.Enqueue(std::move(response));
                    assert(queued);
                }
            },
            *command);
    }
}

void InstanceCommandQueue::RecordOverflow() noexcept
{
    overflowCount_.fetch_add(1, std::memory_order_relaxed);
}

} // namespace consolidator::core

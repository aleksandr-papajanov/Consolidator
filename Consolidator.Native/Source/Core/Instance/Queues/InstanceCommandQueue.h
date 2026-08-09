#pragma once

#include <atomic>
#include <cstddef>

#include "Core/Commands/Commands.h"
#include "Core/Commands/SpscQueue.h"

namespace consolidator::core
{

class ConsolidatorInstance;
class InstanceResponseQueue;

class InstanceCommandQueue
{
public:
    [[nodiscard]] bool TryEnqueue(Command command);
    void Process(
        ConsolidatorInstance& instance,
        InstanceResponseQueue& responseQueue);
    void RecordOverflow() noexcept;

private:
    SpscQueue<Command, 128> queue_;
    std::atomic<std::size_t> overflowCount_{0};
};

} // namespace consolidator::core

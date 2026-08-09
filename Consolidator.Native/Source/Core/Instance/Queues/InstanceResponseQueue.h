#pragma once

#include <cstddef>
#include <optional>

#include "Core/Commands/SpscQueue.h"
#include "Core/Notifications/Notifications.h"

namespace consolidator::core
{

class InstanceResponseQueue
{
public:
    [[nodiscard]] bool CanEnqueue() const noexcept;
    [[nodiscard]] bool Enqueue(StateResponse response) noexcept;
    [[nodiscard]] std::optional<StateResponse> TryDequeue();

private:
    static constexpr std::size_t kCapacity = 128;

    SpscQueue<StateResponse, kCapacity> queue_;
};

} // namespace consolidator::core

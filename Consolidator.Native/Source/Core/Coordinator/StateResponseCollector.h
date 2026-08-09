#pragma once

#include <cstdint>

#include "Core/Commands/CommandQueue.h"
#include "Core/Notifications/Notifications.h"

namespace consolidator::core
{

class StateResponseCollector
{
public:
    StateResponseCollector(
        CommandQueue<StateResponse>& output,
        std::uint16_t responseCount) noexcept;

    void Publish(StateResponse response, std::uint16_t responseIndex);

private:
    CommandQueue<StateResponse>& output_;
    std::uint16_t responseCount_;
};

} // namespace consolidator::core

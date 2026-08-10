#pragma once

#include <cstdint>

#include "Core/Queues/ConcurrentQueue.h"
#include "Core/Domain/Commands/StateProtocolCommands.h"

namespace consolidator::core
{

class StateResponsePublisher
{
public:
    StateResponsePublisher(
        ConcurrentQueue<StateResponse>& output,
        std::uint16_t responseCount) noexcept;

    void Publish(StateResponse response, std::uint16_t responseIndex);

private:
    ConcurrentQueue<StateResponse>& output_;
    std::uint16_t responseCount_;
};

} // namespace consolidator::core

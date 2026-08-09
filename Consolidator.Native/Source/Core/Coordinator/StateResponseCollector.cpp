#include "Core/Coordinator/StateResponseCollector.h"

#include <utility>

namespace consolidator::core
{

StateResponseCollector::StateResponseCollector(
    CommandQueue<StateResponse>& output,
    std::uint16_t responseCount) noexcept
    : output_(output)
    , responseCount_(responseCount)
{
}

void StateResponseCollector::Publish(
    StateResponse response,
    std::uint16_t responseIndex)
{
    response.responseIndex = responseIndex;
    response.responseCount = responseCount_;
    response.isFinal = responseIndex + 1 == responseCount_;
    response.truncated = response.entries.truncated;
    output_.Enqueue(std::move(response));
}

} // namespace consolidator::core

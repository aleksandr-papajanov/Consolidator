#include "Core/Coordinator/Routing/StateResponsePublisher.h"

#include <utility>

namespace consolidator::core
{

StateResponsePublisher::StateResponsePublisher(
    ConcurrentQueue<StateResponse>& output,
    std::uint16_t responseCount) noexcept
    : output_(output)
    , responseCount_(responseCount)
{
}

void StateResponsePublisher::Publish(
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

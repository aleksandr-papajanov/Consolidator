#include "DspStateConsumer.h"

#include <atomic>

namespace consolidator::max
{

bool ConsumePublishedDspState(
    DspStateExchange& exchange,
    std::uint32_t& consumerIndex,
    DspSnapshot& destination) noexcept
{
    const auto publishedIndex =
        std::atomic_ref{ exchange.publishedIndex }
            .load(std::memory_order_acquire);

    if (publishedIndex > 2 ||
        publishedIndex == consumerIndex)
    {
        return false;
    }

    std::atomic_ref{ exchange.consumerIndex }
        .store(
            publishedIndex,
            std::memory_order_release);
    destination = exchange.snapshots[publishedIndex];
    consumerIndex = publishedIndex;
    return true;
}

} // namespace consolidator::max

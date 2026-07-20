#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <utility>

namespace consolidator::dspcore {

// One producer and one consumer exchange the newest complete value without blocking.
template <typename Value>
class LatestValueTripleBuffer final {
public:
    Value& ProducerValue() noexcept {
        return values[producerIndex];
    }

    void Publish() noexcept {
        const auto previous = middleIndex.exchange(
            producerIndex | DirtyMask,
            std::memory_order_acq_rel);
        producerIndex = previous & IndexMask;
        if ((previous & DirtyMask) != 0) {
            replacedCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    template <typename Consumer>
    bool ConsumeLatest(Consumer&& consumer) {
        if (!HasPending()) return false;

        const auto previous = middleIndex.exchange(consumerIndex, std::memory_order_acq_rel);
        if ((previous & DirtyMask) == 0) return false;

        consumerIndex = previous & IndexMask;
        std::invoke(std::forward<Consumer>(consumer), std::as_const(values[consumerIndex]));
        return true;
    }

    bool DiscardLatest() {
        return ConsumeLatest([](const Value&) {});
    }

    bool HasPending() const noexcept {
        return (middleIndex.load(std::memory_order_acquire) & DirtyMask) != 0;
    }

    std::uint64_t ReplacedCount() const noexcept {
        return replacedCount.load(std::memory_order_relaxed);
    }

private:
    static constexpr unsigned DirtyMask = 1U << 31U;
    static constexpr unsigned IndexMask = ~DirtyMask;

    std::array<Value, 3> values;
    std::atomic<unsigned> middleIndex{ 1 };
    std::atomic<std::uint64_t> replacedCount{ 0 };
    unsigned consumerIndex = 0;
    unsigned producerIndex = 2;
};

} // namespace consolidator::dspcore

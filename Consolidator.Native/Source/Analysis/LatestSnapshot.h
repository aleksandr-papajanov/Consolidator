#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace consolidator::analysis
{

// Lock-free latest-input transport.
// Intermediate revisions may be dropped intentionally.
// Each mailbox has exactly one logical producer and one logical consumer.
// A read consumes the published storage and releases it for reuse. Use
// LatestValue for persistent worker results and UI readers.
template <typename T>
class LatestSnapshot final
{
public:
    void Publish(const T& value) noexcept
    {
        for (auto& buffer : buffers_)
        {
            auto expected = State::Free;
            if (buffer.state.compare_exchange_strong(expected, State::Writing,
                    std::memory_order_acquire, std::memory_order_relaxed))
            {
                buffer.value = value;
                buffer.state.store(State::Published, std::memory_order_release);
                return;
            }
        }

        for (auto& buffer : buffers_)
        {
            auto expected = State::Published;
            if (buffer.state.compare_exchange_strong(expected, State::Writing,
                    std::memory_order_acquire, std::memory_order_relaxed))
            {
                buffer.value = value;
                buffer.state.store(State::Published, std::memory_order_release);
                return;
            }
        }
    }

    [[nodiscard]] bool TryReadNewerThan(
        T& value,
        std::uint64_t processedRevision) const noexcept
    {
        T latestValue{};
        bool found = false;
        for (auto& buffer : buffers_)
        {
            auto expected = State::Published;
            if (!buffer.state.compare_exchange_strong(expected, State::Reading,
                    std::memory_order_acquire, std::memory_order_relaxed))
            {
                continue;
            }

            if (buffer.value.revision > processedRevision &&
                (!found || buffer.value.revision > latestValue.revision))
            {
                latestValue = buffer.value;
                found = true;
            }
            buffer.state.store(State::Free, std::memory_order_release);
        }

        if (!found)
        {
            return false;
        }

        value = std::move(latestValue);
        return true;
    }

private:
    enum class State : std::uint8_t { Free, Writing, Published, Reading };

    struct Buffer
    {
        T value{};
        std::atomic<State> state{State::Free};
    };

    static constexpr std::size_t kBufferCount = 3;
    // Reading a snapshot is logically const, but acquiring/releasing the
    // reader-owned state mutates the transport protocol.
    mutable std::array<Buffer, kBufferCount> buffers_{};
};

} // namespace consolidator::analysis

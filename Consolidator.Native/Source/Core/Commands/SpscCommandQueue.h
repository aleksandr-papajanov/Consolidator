#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <utility>

namespace consolidator::core
{

template <typename T, std::size_t Capacity>
class SpscCommandQueue
{
public:
    static_assert(Capacity > 1);

    [[nodiscard]] bool TryEnqueue(T command)
    {
        const auto writeIndex = writeIndex_.load(std::memory_order_relaxed);
        const auto nextWriteIndex = Next(writeIndex);
        if (nextWriteIndex == readIndex_.load(std::memory_order_acquire))
        {
            return false;
        }

        commands_[writeIndex].emplace(std::move(command));
        writeIndex_.store(nextWriteIndex, std::memory_order_release);
        return true;
    }

    [[nodiscard]] std::optional<T> TryDequeue()
    {
        const auto readIndex = readIndex_.load(std::memory_order_relaxed);
        if (readIndex == writeIndex_.load(std::memory_order_acquire))
        {
            return std::nullopt;
        }

        auto command = std::move(*commands_[readIndex]);
        commands_[readIndex].reset();
        readIndex_.store(Next(readIndex), std::memory_order_release);
        return command;
    }

private:
    [[nodiscard]] static constexpr std::size_t Next(std::size_t index) noexcept
    {
        return (index + 1) % Capacity;
    }

    std::array<std::optional<T>, Capacity> commands_;
    std::atomic<std::size_t> writeIndex_{0};
    std::atomic<std::size_t> readIndex_{0};
};

} // namespace consolidator::core

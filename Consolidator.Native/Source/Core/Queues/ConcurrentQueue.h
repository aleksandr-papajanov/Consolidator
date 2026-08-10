#pragma once

#include <mutex>
#include <optional>
#include <queue>
#include <utility>

namespace consolidator::core
{

template <typename T>
class ConcurrentQueue
{
public:
    void Enqueue(T command)
    {
        std::lock_guard lock{mutex_};
        commands_.push(std::move(command));
    }

    [[nodiscard]] std::optional<T> TryDequeue()
    {
        std::lock_guard lock{mutex_};
        if (commands_.empty())
        {
            return std::nullopt;
        }

        auto command = std::move(commands_.front());
        commands_.pop();
        return command;
    }

    [[nodiscard]] bool HasCommands() const
    {
        std::lock_guard lock{mutex_};
        return !commands_.empty();
    }

private:
    mutable std::mutex mutex_;
    std::queue<T> commands_;
};

} // namespace consolidator::core

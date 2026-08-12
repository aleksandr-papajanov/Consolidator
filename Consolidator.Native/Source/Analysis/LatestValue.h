#pragma once

#include <cstdint>
#include <mutex>

namespace consolidator::analysis
{

// Persistent latest-value storage for worker results and UI readers.
// Reading does not remove the value, so multiple readers can inspect the
// same result and a later reader can reopen an unchanged view.
template <typename T>
class LatestValue final
{
  public:
    void Publish(const T& value)
    {
        std::lock_guard lock{mutex_};
        value_ = value;
        hasValue_ = true;
    }

    [[nodiscard]] bool ReadLatest(T& value) const
    {
        std::lock_guard lock{mutex_};
        if (!hasValue_)
        {
            return false;
        }

        value = value_;
        return true;
    }

    [[nodiscard]] bool TryReadNewerThan(
        T& value,
        std::uint64_t revision) const
    {
        std::lock_guard lock{mutex_};
        if (!hasValue_ || value_.revision <= revision)
        {
            return false;
        }

        value = value_;
        return true;
    }

  private:
    mutable std::mutex mutex_;
    T value_{};
    bool hasValue_ = false;
};

} // namespace consolidator::analysis

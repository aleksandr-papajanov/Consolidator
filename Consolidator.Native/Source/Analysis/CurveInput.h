#pragma once

#include <cstdint>
#include <array>
#include <mutex>

namespace consolidator::analysis
{

// Immutable instance state published for on-demand curve calculation.
struct CurveFilterState
{
    float frequencyHz = 0.0F;
    float q = 1.0F;
    float gainDb = 0.0F;
    bool bypass = false;
    bool solo = false;
};

struct CurveBankState
{
    bool bypass = false;
    bool solo = false;
    std::array<CurveFilterState, 7> filters{};
};

struct CurveInput
{
    std::array<CurveBankState, 7> banks{};
    bool equalizerActive = false;
    bool chainAllowsEqualizer = false;
    double sampleRate = 0.0;
    std::uint64_t revision = 0;
};

// Keeps the newest curve state available for repeated view changes.
class CurveState final
{
  public:
    void Publish(const CurveInput& value) noexcept
    {
        std::lock_guard lock{mutex_};
        value_ = value;
    }

    [[nodiscard]] CurveInput Read() const
    {
        std::lock_guard lock{mutex_};
        return value_;
    }

  private:
    mutable std::mutex mutex_;
    CurveInput value_;
};

} // namespace consolidator::analysis

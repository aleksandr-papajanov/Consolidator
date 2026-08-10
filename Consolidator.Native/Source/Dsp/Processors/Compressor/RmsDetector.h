#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace consolidator::dsp
{

struct RmsDetectorMeterState
{
    std::atomic<float> levelLinear{0.0f};
};

// Computes a short-window RMS level and exposes it as an atomic meter snapshot.
class RmsDetector
{
public:
    static constexpr std::size_t kWindowSize = 64;

    // Adds one sample to the rolling window and returns its linear RMS level.
    [[nodiscard]] double ProcessSample(double sample) noexcept;

    void Reset() noexcept;

    [[nodiscard]] float GetLevelLinear() const noexcept
    {
        return meterState_.levelLinear.load(std::memory_order_relaxed);
    }

private:
    std::array<double, kWindowSize> squaredSamples_{};

    std::size_t writeIndex_ = 0;
    std::size_t sampleCount_ = 0;

    double squaredSum_ = 0.0;

    RmsDetectorMeterState meterState_;
};

} // namespace consolidator::dsp

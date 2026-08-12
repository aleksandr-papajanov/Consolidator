#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace consolidator::dsp
{

// Applies block-size-independent exponential smoothing to one meter value.
class MeterSmoother final
{
  public:
    explicit MeterSmoother(float initialValue = 0.0f) noexcept
        : value_(initialValue), initialValue_(initialValue)
    {
    }

    void SetSampleRate(double sampleRate) noexcept
    {
        sampleRate_ = std::max(sampleRate, 1.0);
    }

    [[nodiscard]] float Process(float current, std::size_t frameCount) noexcept
    {
        const double blockDuration =
            static_cast<double>(frameCount) / sampleRate_;
        const float coefficient = static_cast<float>(std::exp(
            -blockDuration / smoothingTimeSeconds_));
        value_ = coefficient * value_ + (1.0f - coefficient) * current;
        return value_;
    }

    void Reset() noexcept
    {
        value_ = initialValue_;
    }

  private:
    static constexpr double smoothingTimeSeconds_ = 0.150;

    double sampleRate_ = 48000.0;
    float value_;
    float initialValue_;
};

} // namespace consolidator::dsp

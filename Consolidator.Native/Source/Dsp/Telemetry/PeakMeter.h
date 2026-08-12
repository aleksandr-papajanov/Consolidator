#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace consolidator::dsp
{

// Keeps a block peak visible across dropped UI snapshots with hold and release.
class PeakMeter final
{
  public:
    void SetSampleRate(double sampleRate) noexcept
    {
        sampleRate_ = std::max(sampleRate, 1.0);
    }

    [[nodiscard]] float Process(float blockPeak, std::size_t frameCount) noexcept
    {
        const double blockDuration =
            static_cast<double>(frameCount) / sampleRate_;
        if (blockPeak >= value_)
        {
            value_ = blockPeak;
            holdRemainingSeconds_ = holdTimeSeconds_;
            return value_;
        }

        if (holdRemainingSeconds_ > 0.0)
        {
            if (blockDuration <= holdRemainingSeconds_)
            {
                holdRemainingSeconds_ -= blockDuration;
                return value_;
            }

            const auto releaseDuration =
                blockDuration - holdRemainingSeconds_;
            holdRemainingSeconds_ = 0.0;
            value_ = static_cast<float>(value_ * std::exp(
                -releaseDuration / releaseTimeSeconds_));
            return value_;
        }

        value_ = static_cast<float>(value_ * std::exp(
            -blockDuration / releaseTimeSeconds_));
        return value_;
    }

    void Reset() noexcept
    {
        value_ = 0.0f;
        holdRemainingSeconds_ = 0.0;
    }

  private:
    static constexpr double holdTimeSeconds_ = 0.075;
    static constexpr double releaseTimeSeconds_ = 0.300;

    double sampleRate_ = 48000.0;
    double holdRemainingSeconds_ = 0.0;
    float value_ = 0.0f;
};

} // namespace consolidator::dsp

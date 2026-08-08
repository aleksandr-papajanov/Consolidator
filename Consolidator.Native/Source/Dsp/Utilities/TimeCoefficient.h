#pragma once

#include <algorithm>
#include <cmath>

namespace consolidator::dsp
{

// Computes a one-pole smoother coefficient for the given time constant.
// `timeMs` is the time to reach ~63% of the target (single time-constant),
// `sampleRate` is in Hz.
//
// Result is in [0, 1): closer to 1.0 means slower response.
[[nodiscard]] inline double CalculateTimeCoefficient(
    double timeMs,
    double sampleRate) noexcept
{
    constexpr double kMinimumTimeMs = 0.01;
    constexpr double kMinimumSampleRate = 1.0;
    constexpr double kMillisecondsPerSecond = 1000.0;

    const double safeTimeMs = std::max(timeMs, kMinimumTimeMs);
    const double safeSampleRate = std::max(sampleRate, kMinimumSampleRate);
    const double timeSeconds = safeTimeMs / kMillisecondsPerSecond;

    return std::exp(-1.0 / (timeSeconds * safeSampleRate));
}

} // namespace consolidator::dsp
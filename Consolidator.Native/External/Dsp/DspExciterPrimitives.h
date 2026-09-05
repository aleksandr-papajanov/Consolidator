#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace consolidator::max::dsp
{

namespace exciter
{

[[nodiscard]] inline double FilterCoefficient(double frequency, double sampleRate) noexcept
{
    const auto tangent = std::tan(3.141592653589793 * std::min(frequency, sampleRate * 0.4) / sampleRate);
    return tangent / (1.0 + tangent);
}

[[nodiscard]] inline double TimeCoefficient(double seconds, double sampleRate) noexcept
{
    return 1.0 - std::exp(-1.0 / (seconds * sampleRate));
}

[[nodiscard]] inline double LowPass(
    double sample, double coefficient, std::array<double, 2>& state) noexcept
{
    for (auto& value : state)
    {
        const auto delta = coefficient * (sample - value);
        const auto low = value + delta;
        value = low + delta;
        sample = low;
    }
    return sample;
}

template <std::size_t StageCount>
[[nodiscard]] inline double HighPass(
    double sample, double coefficient, std::array<double, StageCount>& state) noexcept
{
    for (auto& value : state)
    {
        const auto delta = coefficient * (sample - value);
        const auto low = value + delta;
        value = low + delta;
        sample -= low;
    }
    return sample;
}

[[nodiscard]] inline double Harmonics(double sample) noexcept
{
    const auto square = sample * sample;
    return square * (0.5 + 0.12 * sample) / (1.0 + square);
}

[[nodiscard]] inline double HarmonicsIntegral(double sample) noexcept
{
    const auto square = sample * sample;
    return 0.5 * (sample - std::atan(sample)) +
        0.06 * (square - std::log1p(square));
}

[[nodiscard]] inline double Generate(double sample, double scale, double& previousSample) noexcept
{
    const auto current = sample / scale;
    const auto previous = previousSample / scale;
    previousSample = sample;
    const auto difference = current - previous;
    // The midpoint is the continuous limit of the ADAA quotient. Use it
    // for nearby endpoints to avoid subtracting nearly equal integrals.
    if (std::abs(difference) < 1.0e-4 * (1.0 + std::abs(current) + std::abs(previous)))
    {
        return scale * Harmonics(0.5 * (current + previous));
    }
    return scale * (HarmonicsIntegral(current) - HarmonicsIntegral(previous)) / difference;
}

} // namespace exciter

} // namespace consolidator::max::dsp

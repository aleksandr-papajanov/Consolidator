#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace consolidator::max::dsp
{

namespace onepole
{

[[nodiscard]] inline double FilterCoefficient(double frequency, double sampleRate) noexcept
{
    const auto tangent = std::tan(3.141592653589793 * std::min(frequency, sampleRate * 0.4) / sampleRate);
    return tangent / (1.0 + tangent);
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

} // namespace onepole

} // namespace consolidator::max::dsp

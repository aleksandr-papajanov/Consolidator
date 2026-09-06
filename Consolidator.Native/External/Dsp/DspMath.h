#pragma once

#include <cmath>

namespace consolidator::max::dsp
{

// One-pole update weight: state += coefficient * (input - state).
// Callers supply positive seconds and a prepared positive sample rate.
[[nodiscard]] inline double TimeCoefficient(double seconds, double sampleRate) noexcept
{
    return 1.0 - std::exp(-1.0 / (seconds * sampleRate));
}

[[nodiscard]] inline double DbToGain(double decibels) noexcept
{
    return std::pow(10.0, decibels / 20.0);
}

} // namespace consolidator::max::dsp

#pragma once

#include <cmath>

namespace consolidator::max::dsp
{

// History remains in physical sample units. Both endpoints use today's
// normalization and shape parameters, including during automation.
template <typename Shape, typename Integral>
[[nodiscard]] double AntiderivativeAverage(
    double sample, double scale, double& previousSample,
    const Shape& shape, const Integral& integral) noexcept
{
    const auto current = sample / scale;
    const auto previous = previousSample / scale;
    previousSample = sample;
    const auto difference = current - previous;
    if (std::abs(difference) < 1.0e-4 * (1.0 + std::abs(current) + std::abs(previous)))
    {
        return scale * shape(0.5 * (current + previous));
    }
    return scale * (integral(current) - integral(previous)) / difference;
}

} // namespace consolidator::max::dsp

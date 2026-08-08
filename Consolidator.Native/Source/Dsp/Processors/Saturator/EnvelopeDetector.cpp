#include "Dsp/Processors/Saturator/EnvelopeDetector.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "Dsp/Processors/Equalizer/Filters/BellFilter.h"
#include "Dsp/Processors/Equalizer/Filters/LowShelfFilter.h"

namespace consolidator::dsp
{

EnvelopeDetector::EnvelopeDetector(
    SaturatorDetectorFilterId lowShelfId,
    SaturatorDetectorFilterId bellId)
{
    filters_.AddFilter(std::make_unique<LowShelfFilter>(
        detail::ToFilterId(detail::ToIndex(lowShelfId)),
        100.0));

    filters_.AddFilter(std::make_unique<BellFilter>(
        detail::ToFilterId(detail::ToIndex(bellId)),
        1000.0));
    RecalculateTimeCoefficients();
}

void EnvelopeDetector::Prepare(double sampleRate)
{
    sampleRate_ = std::max(sampleRate, 1.0);

    filters_.Prepare(sampleRate_, 1);

    RecalculateTimeCoefficients();
    Reset();
}

void EnvelopeDetector::Reset() noexcept
{
    filters_.Reset();

    envelope_ = 0.0;
}

double EnvelopeDetector::ProcessSample(double input) noexcept
{
    const double filtered = filters_.ProcessSample(input);
    const double rectified = std::abs(filtered);

    const double coefficient =
        rectified > envelope_
            ? attackCoefficient_
            : releaseCoefficient_;

    envelope_ =
        coefficient * envelope_ +
        (1.0 - coefficient) * rectified;

    return envelope_;
}

bool EnvelopeDetector::ApplyParameter(
    const ParameterRoute& route,
    const ParameterValue& value,
    std::size_t depth)
{
    return filters_.ApplyParameter(route, value, depth);
}

void EnvelopeDetector::SetAttackMs(double attackMs)
{
    settings_.attackMs = std::max(attackMs, kMinimumTimeMs);
    RecalculateTimeCoefficients();
}

void EnvelopeDetector::SetReleaseMs(double releaseMs)
{
    settings_.releaseMs = std::max(releaseMs, kMinimumTimeMs);
    RecalculateTimeCoefficients();
}

void EnvelopeDetector::RecalculateTimeCoefficients() noexcept
{
    attackCoefficient_ = CalculateTimeCoefficient(settings_.attackMs, sampleRate_);
    releaseCoefficient_ = CalculateTimeCoefficient(settings_.releaseMs, sampleRate_);
}

} // namespace consolidator::dsp

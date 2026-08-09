#include "Dsp/Processors/Saturator/DetectorEnvelopeFollower.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "Dsp/Processors/Equalizer/Filters/BellFilter.h"
#include "Dsp/Processors/Equalizer/Filters/LowShelfFilter.h"

namespace consolidator::dsp
{

DetectorEnvelopeFollower::DetectorEnvelopeFollower(
    SaturatorDetectorFilterId lowShelfId,
    SaturatorDetectorFilterId bellId)
{
    filters_.AddFilter(std::make_unique<LowShelfFilter>(
        detail::ToFilterId(detail::ToIndex(lowShelfId)),
        core::settings::DetectorDefaults::kDefaultLowShelfFrequencyHz));

    filters_.AddFilter(std::make_unique<BellFilter>(
        detail::ToFilterId(detail::ToIndex(bellId)),
        core::settings::DetectorDefaults::kDefaultBellFrequencyHz));
    RecalculateTimeCoefficients();
}

void DetectorEnvelopeFollower::Prepare(double sampleRate)
{
    sampleRate_ = std::max(sampleRate, 1.0);

    filters_.Prepare(sampleRate_, 1);

    RecalculateTimeCoefficients();
    Reset();
}

void DetectorEnvelopeFollower::Reset() noexcept
{
    filters_.Reset();

    envelope_ = 0.0;
}

double DetectorEnvelopeFollower::ProcessSample(double input) noexcept
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

bool DetectorEnvelopeFollower::WriteParameter(
    const core::StatePath& route,
    const ParameterValue& value,
    std::size_t depth)
{
    return filters_.WriteParameter(route, value, depth);
}

void DetectorEnvelopeFollower::SetAttackMs(double attackMs)
{
    settings_.attackMs = std::max(
        attackMs,
        core::settings::DetectorDefaults::kMinimumTimeMs);
    RecalculateTimeCoefficients();
}

void DetectorEnvelopeFollower::SetReleaseMs(double releaseMs)
{
    settings_.releaseMs = std::max(
        releaseMs,
        core::settings::DetectorDefaults::kMinimumTimeMs);
    RecalculateTimeCoefficients();
}

void DetectorEnvelopeFollower::RecalculateTimeCoefficients() noexcept
{
    attackCoefficient_ = CalculateTimeCoefficient(settings_.attackMs, sampleRate_);
    releaseCoefficient_ = CalculateTimeCoefficient(settings_.releaseMs, sampleRate_);
}

} // namespace consolidator::dsp

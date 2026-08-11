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
    monitoringSample_ = 0.0;
}

bool DetectorEnvelopeFollower::Reset(
    const core::StatePath& route,
    std::size_t depth) noexcept
{
    if (depth == route.GetDepth())
    {
        Reset();
        return true;
    }

    return filters_.Reset(route, depth);
}

double DetectorEnvelopeFollower::ProcessSample(double input) noexcept
{
    const double filtered = filters_.ProcessSample(input);
    monitoringSample_ = filtered;
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

bool DetectorEnvelopeFollower::ApplyParameter(
    const core::StatePath& route,
    const ParameterVariant& value,
    std::size_t depth)
{
    auto equalizerRoute = route;
    equalizerRoute.deviceId = DeviceId::Equalizer;
    return filters_.ApplyParameter(equalizerRoute, value, depth);
}

bool DetectorEnvelopeFollower::ApplyProcessingStateAtDepth(
    const core::StatePath& target,
    bool active,
    std::size_t depth)
{
    auto equalizerTarget = target;
    equalizerTarget.deviceId = DeviceId::Equalizer;
    return filters_.ApplyProcessingStateAtDepth(equalizerTarget, active, depth);
}

bool DetectorEnvelopeFollower::ApplyMonitoringState(
    const core::StatePath& target,
    bool enabled,
    std::size_t depth)
{
    if (target.GetParameterId() == ParameterId::Listen &&
        depth == target.GetDepth())
    {
        listen_ = enabled;
        return true;
    }
    return false;
}

void DetectorEnvelopeFollower::CommitRuntimeUpdates()
{
    filters_.CommitRuntimeUpdates();
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

#include "Dsp/Processors/Saturator/EnvelopeDetector.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace consolidator::dsp
{

EnvelopeDetector::EnvelopeDetector(
    SaturatorDetectorFilterId lowShelfId,
    SaturatorDetectorFilterId bellId)
    : lowShelf_(detail::ToEqFilterId(detail::ToIndex(lowShelfId)), 100.0)
    , bell_(detail::ToEqFilterId(detail::ToIndex(bellId)), 1000.0)
{
    RecalculateTimeCoefficients();
}

void EnvelopeDetector::Prepare(double sampleRate)
{
    sampleRate_ = std::max(sampleRate, 1.0);

    lowShelf_.Prepare(sampleRate_, 1);
    bell_.Prepare(sampleRate_, 1);

    RecalculateTimeCoefficients();
    Reset();
}

void EnvelopeDetector::Reset() noexcept
{
    lowShelf_.Reset();
    bell_.Reset();

    envelope_ = 0.0;
}

double EnvelopeDetector::ProcessSample(double input) noexcept
{
    const double filtered = bell_.ProcessSample(lowShelf_.ProcessSample(input, 0), 0);
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

void EnvelopeDetector::ApplyParameterChange(
    const ParameterChange& change)
{
    const auto& addr = change.address;

    if (addr.GetElementKind() == detail::ElementKind::SaturatorDetectorFilter)
    {
        if (addr.GetElementIndex() == 0)
        {
            lowShelf_.ApplyParameterChange(change);
        }
        else
        {
            bell_.ApplyParameterChange(change);
        }
    }
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

double EnvelopeDetector::CalculateTimeCoefficient(double timeMs, double sampleRate) noexcept
{
    const double safeTimeMs = std::max(timeMs, kMinimumTimeMs);
    const double safeSampleRate = std::max(sampleRate, 1.0);
    const double timeSeconds = safeTimeMs * 0.001;
    return std::exp(-1.0 / (timeSeconds * safeSampleRate));
}

} // namespace consolidator::dsp
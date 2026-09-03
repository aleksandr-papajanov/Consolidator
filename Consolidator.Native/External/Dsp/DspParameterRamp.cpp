#include "DspParameterRamp.h"

#include <algorithm>
#include <cmath>

namespace consolidator::max::dsp
{

DspParameterRamp::DspParameterRamp(double initialValue) noexcept
    : currentValue_(initialValue),
      targetValue_(initialValue)
{
}

void DspParameterRamp::Prepare(
    const DspProcessingContext& context) noexcept
{
    const auto rampSamples = std::round(context.sampleRate * 0.01);
    rampSampleCount_ = static_cast<std::size_t>(
        std::max(1.0, rampSamples));
}

void DspParameterRamp::Reset() noexcept
{
    currentValue_ = targetValue_;
    step_ = 0.0;
    remainingSamples_ = 0;
}

void DspParameterRamp::SetTarget(double targetValue) noexcept
{
    if (targetValue == targetValue_)
    {
        return;
    }

    targetValue_ = targetValue;
    if (targetValue_ == currentValue_)
    {
        step_ = 0.0;
        remainingSamples_ = 0;
        return;
    }

    remainingSamples_ = rampSampleCount_;
    step_ = (targetValue_ - currentValue_) /
        static_cast<double>(remainingSamples_);
}

double DspParameterRamp::NextValue() noexcept
{
    const auto value = currentValue_;
    Advance(1);
    return value;
}

void DspParameterRamp::Advance(std::size_t sampleCount) noexcept
{
    if (remainingSamples_ == 0 || sampleCount == 0)
    {
        return;
    }

    if (sampleCount >= remainingSamples_)
    {
        currentValue_ = targetValue_;
        step_ = 0.0;
        remainingSamples_ = 0;
        return;
    }

    currentValue_ += step_ * static_cast<double>(sampleCount);
    remainingSamples_ -= sampleCount;
}

double DspParameterRamp::CurrentValue() const noexcept
{
    return currentValue_;
}

double DspParameterRamp::TargetValue() const noexcept
{
    return targetValue_;
}

bool DspParameterRamp::IsSmoothing() const noexcept
{
    return remainingSamples_ > 0;
}

} // namespace consolidator::max::dsp

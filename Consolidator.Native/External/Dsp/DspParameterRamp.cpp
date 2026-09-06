#include "DspParameterRamp.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

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

} // namespace consolidator::max::dsp

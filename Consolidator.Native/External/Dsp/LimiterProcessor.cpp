#include "LimiterProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "DspMath.h"
#include "DspProcessingPrimitives.h"

namespace consolidator::max::dsp
{

void LimiterProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    targetDb_.Prepare(context);
    releaseCoefficient_ = TimeCoefficient(0.080, context.sampleRate);
}

void LimiterProcessor::Reset() noexcept
{
    targetDb_.Reset();
    gain_ = 1.0;
}

void LimiterProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    targetDb_.SetTarget(snapshot.outputLimit);
    enabled_ = snapshot.outputLimiter != 0;
}

void LimiterProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    const auto channels = SelectChannels(frame.main, frame.plan.channelMode);
    const auto fixedCeiling = DbToGain(targetDb_.TargetValue());
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        // A lower target is a hard ceiling from this block onward.
        // Raising the ceiling retains the ordinary parameter ramp.
        const auto ceiling = targetDb_.IsSmoothing()
            ? DbToGain(std::min(targetDb_.NextValue(), targetDb_.TargetValue()))
            : fixedCeiling;
        const auto peak = channels.Peak(index);
        const auto requiredGain = enabled_ && peak > ceiling ? ceiling / peak : 1.0;
        gain_ = requiredGain < gain_ ? requiredGain : gain_ + releaseCoefficient_ * (requiredGain - gain_);
        if (requiredGain == 1.0 && gain_ > 1.0 - 1.0e-9)
        {
            gain_ = 1.0;
        }
        if (channels.left != nullptr)
        {
            frame.main.left[index] *= gain_;
            if (enabled_)
            {
                frame.main.left[index] = std::clamp(frame.main.left[index], -ceiling, ceiling);
            }
        }
        if (channels.right != nullptr)
        {
            frame.main.right[index] *= gain_;
            if (enabled_)
            {
                frame.main.right[index] = std::clamp(frame.main.right[index], -ceiling, ceiling);
            }
        }
    }
}

bool LimiterProcessor::RequiresProcessing() const noexcept
{
    return enabled_ || gain_ < 1.0;
}

} // namespace consolidator::max::dsp

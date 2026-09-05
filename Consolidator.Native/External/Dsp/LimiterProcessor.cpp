#include "DspProcessors.h"

#include <algorithm>
#include <cmath>

namespace consolidator::max::dsp
{

void LimiterProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    targetDb_.Prepare(context);
    releaseCoefficient_ = 1.0 - std::exp(-1.0 / (context.sampleRate * 0.080));
}

void LimiterProcessor::Reset() noexcept
{
    targetDb_.Reset();
    gain_ = 1.0;
}

void LimiterProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    targetDb_.SetTarget(snapshot.outputTarget);
    enabled_ = snapshot.outputLimiter != 0;
}

void LimiterProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    const auto left = frame.plan.channelMode != DspChannelMode::Right && frame.main.left != nullptr;
    const auto right = frame.plan.channelMode != DspChannelMode::Left && frame.main.right != nullptr;
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        // A lower target is a hard ceiling from this block onward.
        // Raising the ceiling retains the ordinary parameter ramp.
        const auto ceilingDb = std::min(targetDb_.NextValue(), targetDb_.TargetValue());
        const auto ceiling = std::pow(10.0, ceilingDb / 20.0);
        auto peak = 0.0;
        if (left)
        {
            peak = std::abs(frame.main.left[index]);
        }
        if (right)
        {
            peak = std::max(peak, std::abs(frame.main.right[index]));
        }
        const auto requiredGain = enabled_ && peak > ceiling ? ceiling / peak : 1.0;
        gain_ = requiredGain < gain_ ? requiredGain : gain_ + releaseCoefficient_ * (requiredGain - gain_);
        if (requiredGain == 1.0 && gain_ > 1.0 - 1.0e-9)
        {
            gain_ = 1.0;
        }
        if (left)
        {
            frame.main.left[index] *= gain_;
            if (enabled_)
            {
                frame.main.left[index] = std::clamp(frame.main.left[index], -ceiling, ceiling);
            }
        }
        if (right)
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

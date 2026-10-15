#include "LimiterProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "../../Primitives/Math.h"
#include "../../Primitives/ChannelView.h"

namespace consolidator::audio::algorithms
{

void LimiterProcessor::Prepare(
    const ProcessingContext& context) noexcept
{
    targetDb_.Prepare(context);
    releaseCoefficient_ = primitives::TimeCoefficient(0.080, context.sampleRate);
}

void LimiterProcessor::Reset() noexcept
{
    targetDb_.Reset();
    gain_ = 1.0;
    reductionDb_ = 0.0;
}

void LimiterProcessor::SetControls(
    double ceilingDb, bool enabled) noexcept
{
    targetDb_.SetTarget(ceilingDb);
    enabled_ = enabled;
}

void LimiterProcessor::Process(
    AudioBuffer audio, ChannelMode mode) noexcept
{
    reductionDb_ = 0.0;
    const auto channels = primitives::SelectChannels(audio, mode);
    auto minimumGain = 1.0;
    const auto fixedCeiling = primitives::DbToGain(targetDb_.TargetValue());
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        // A lower target is a hard ceiling from this block onward.
        // Raising the ceiling retains the ordinary parameter ramp.
        const auto ceiling = targetDb_.IsSmoothing()
            ? primitives::DbToGain(std::min(targetDb_.NextValue(), targetDb_.TargetValue()))
            : fixedCeiling;
        const auto peak = channels.Peak(index);
        const auto requiredGain = enabled_ && peak > ceiling ? ceiling / peak : 1.0;
        gain_ = requiredGain < gain_ ? requiredGain : gain_ + releaseCoefficient_ * (requiredGain - gain_);
        if (requiredGain == 1.0 && gain_ > 1.0 - 1.0e-9)
        {
            gain_ = 1.0;
        }
        minimumGain = std::min(minimumGain, gain_);
        if (channels.left != nullptr)
        {
            audio.left[index] *= gain_;
            if (enabled_)
            {
                audio.left[index] = std::clamp(audio.left[index], -ceiling, ceiling);
            }
        }
        if (channels.right != nullptr)
        {
            audio.right[index] *= gain_;
            if (enabled_)
            {
                audio.right[index] = std::clamp(audio.right[index], -ceiling, ceiling);
            }
        }
    }
    reductionDb_ = -20.0 * std::log10(std::max(minimumGain, 1.0e-12));
}

double LimiterProcessor::ReductionDb() const noexcept
{
    return reductionDb_;
}

bool LimiterProcessor::RequiresProcessing() const noexcept
{
    return enabled_ || gain_ < 1.0;
}

} // namespace consolidator::audio::algorithms

#include "DspProcessors.h"

#include <cmath>

namespace consolidator::max::dsp
{

void DspGainRamp::Prepare(
    const DspProcessingContext& context) noexcept
{
    gain_.Prepare(context);
}

void DspGainRamp::Reset() noexcept
{
    gain_.Reset();
}

void DspGainRamp::SetTargetDb(float targetDb) noexcept
{
    gain_.SetTarget(std::pow(
        10.0,
        static_cast<double>(targetDb) / 20.0));
}

void DspGainRamp::Process(
    DspAudioBuffer& audio,
    DspChannelMode channelMode) noexcept
{
    if (!RequiresProcessing())
    {
        return;
    }

    const auto processLeft = channelMode != DspChannelMode::Right;
    const auto processRight = channelMode != DspChannelMode::Left;
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto gain = gain_.NextValue();
        if (processLeft && audio.left != nullptr)
        {
            audio.left[index] *= gain;
        }
        if (processRight && audio.right != nullptr)
        {
            audio.right[index] *= gain;
        }
    }
}

bool DspGainRamp::RequiresProcessing() const noexcept
{
    return gain_.IsSmoothing() || gain_.CurrentValue() != 1.0;
}

} // namespace consolidator::max::dsp

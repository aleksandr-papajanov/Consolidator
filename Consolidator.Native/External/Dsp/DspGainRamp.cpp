#include "DspGainRamp.h"

#include <cstddef>

#include "DspMath.h"
#include "DspProcessingPrimitives.h"

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
    gain_.SetTarget(DbToGain(targetDb));
}

void DspGainRamp::Process(
    DspAudioBuffer& audio,
    DspChannelMode channelMode) noexcept
{
    if (!RequiresProcessing())
    {
        return;
    }

    const auto channels = SelectChannels(audio, channelMode);
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto gain = gain_.NextValue();
        channels.ApplyGain(index, gain);
    }
}

bool DspGainRamp::RequiresProcessing() const noexcept
{
    return gain_.IsSmoothing() || gain_.CurrentValue() != 1.0;
}

} // namespace consolidator::max::dsp

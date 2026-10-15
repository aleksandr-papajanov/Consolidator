#include "GainRamp.h"

#include <cstddef>

#include "Math.h"
#include "ChannelView.h"

namespace consolidator::audio::primitives
{

void GainRamp::Prepare(
    const ProcessingContext& context) noexcept
{
    gain_.Prepare(context);
}

void GainRamp::Reset() noexcept
{
    gain_.Reset();
}

void GainRamp::SetTargetDb(float targetDb) noexcept
{
    gain_.SetTarget(DbToGain(targetDb));
}

void GainRamp::Process(
    AudioBuffer& audio,
    ChannelMode channelMode) noexcept
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

bool GainRamp::RequiresProcessing() const noexcept
{
    return gain_.IsSmoothing() || gain_.CurrentValue() != 1.0;
}

} // namespace consolidator::audio::primitives

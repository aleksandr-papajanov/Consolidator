#include "RmsMeter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "ChannelView.h"
#include "Math.h"

namespace consolidator::audio::primitives
{

void RmsMeter::Prepare(const ProcessingContext& context) noexcept
{
    coefficient_ = TimeCoefficient(0.300, context.sampleRate);
}

void RmsMeter::Reset() noexcept
{
    energy_ = 0.0;
}

void RmsMeter::Process(AudioBuffer audio, ChannelMode mode) noexcept
{
    const auto channels = SelectChannels(audio, mode);
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        energy_ += coefficient_ * (channels.MeanSquare(index) - energy_);
    }
    if (!std::isfinite(energy_))
    {
        energy_ = 0.0;
    }
}

double RmsMeter::LevelDb() const noexcept
{
    return 10.0 * std::log10(std::max(energy_, 1.0e-12));
}

} // namespace consolidator::audio::primitives

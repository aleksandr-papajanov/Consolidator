#include "ShaperDevice.h"

namespace consolidator::audio::devices
{

void ShaperDevice::Prepare(const ProcessingContext& context)
{
    shaper_.Prepare(context);
    transition_.Prepare(context);
}

void ShaperDevice::Reset() noexcept
{
    shaper_.Reset();
    transition_.Reset();
}

void ShaperDevice::SetParameters(const ShaperParameters& parameters) noexcept
{
    shaper_.SetAmounts(parameters.attack, parameters.sustain);
    shaper_.SetOutputGainDb(parameters.outputDb);
    const auto active = !parameters.bypass &&
        (parameters.attack != 0.0 || parameters.sustain != 0.0 || parameters.outputDb != 0.0);
    transition_.SetTarget(active ? 1.0 : 0.0);
}

void ShaperDevice::Process(AudioBuffer audio, ChannelMode mode) noexcept
{
    transition_.Process(audio, mode, [&](AudioBuffer wet)
    {
        shaper_.Process(wet, mode);
    }, [&]()
    {
        shaper_.Reset();
    });
}

} // namespace consolidator::audio::devices

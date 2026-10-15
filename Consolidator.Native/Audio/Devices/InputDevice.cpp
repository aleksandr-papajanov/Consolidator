#include "InputDevice.h"

namespace consolidator::audio::devices
{

void InputDevice::Prepare(const ProcessingContext& context)
{
    width_.Prepare(context);
    gain_.Prepare(context);
    transition_.Prepare(context);
}

void InputDevice::Reset() noexcept
{
    width_.Reset();
    gain_.Reset();
    transition_.Reset();
}

void InputDevice::SetParameters(const InputParameters& parameters) noexcept
{
    width_.SetAmount(parameters.width);
    gain_.SetParameters(parameters.gain);
    transition_.SetTarget(parameters.bypass ? 0.0 : 1.0);
}

void InputDevice::Process(AudioBuffer audio, ChannelMode mode) noexcept
{
    transition_.Process(audio, mode, [&](AudioBuffer wet)
    {
        width_.Process(wet, mode);
        gain_.Process(wet, mode);
    }, [&]()
    {
        width_.Reset();
        gain_.Reset();
    });
}

GainStageTelemetry InputDevice::Telemetry() const noexcept
{
    return gain_.Telemetry();
}

} // namespace consolidator::audio::devices

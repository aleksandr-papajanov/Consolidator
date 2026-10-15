#include "PolishDevice.h"

namespace consolidator::audio::devices
{

void PolishDevice::Prepare(const ProcessingContext& context)
{
    thick_.Prepare(context);
    air_.Prepare(context);
    transition_.Prepare(context);
}

void PolishDevice::Reset() noexcept
{
    thick_.Reset();
    air_.Reset();
    transition_.Reset();
}

void PolishDevice::SetParameters(const PolishParameters& parameters) noexcept
{
    thick_.SetAmount(parameters.thick);
    air_.SetAmount(parameters.air);
    const auto active = !parameters.bypass && (parameters.thick != 0.0 || parameters.air != 0.0);
    transition_.SetTarget(active ? 1.0 : 0.0);
}

void PolishDevice::Process(AudioBuffer audio, ChannelMode mode) noexcept
{
    transition_.Process(audio, mode, [&](AudioBuffer wet)
    {
        thick_.Process(wet, mode);
        air_.Process(wet, mode);
    }, [&]()
    {
        thick_.Reset();
        air_.Reset();
    });
}

} // namespace consolidator::audio::devices

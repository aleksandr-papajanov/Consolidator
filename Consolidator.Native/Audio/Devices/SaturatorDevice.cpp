#include "SaturatorDevice.h"

namespace consolidator::audio::devices
{

void SaturatorDevice::Prepare(const ProcessingContext& context)
{
    detector_.Prepare(context);
    saturation_.Prepare(context);
    output_.Prepare(context);
    transition_.Prepare(context);
}

void SaturatorDevice::Reset() noexcept
{
    detector_.Reset();
    saturation_.Reset();
    output_.Reset();
    transition_.Reset();
}

void SaturatorDevice::SetParameters(const SaturatorParameters& parameters) noexcept
{
    detector_.SetParameters(parameters.detector);
    saturation_.SetControls(parameters.drive, parameters.curve, parameters.split);
    output_.SetTargetDb(static_cast<float>(parameters.outputDb));
    const auto active = !parameters.bypass && (parameters.drive != 0.0 || parameters.outputDb != 0.0);
    transition_.SetTarget(active ? 1.0 : 0.0);
}

void SaturatorDevice::Process(AudioBuffer audio, ChannelMode mode) noexcept
{
    transition_.Process(audio, mode, [&](AudioBuffer wet)
    {
        const auto detector = detector_.Capture(wet, mode);
        saturation_.Process(wet, detector, mode);
        output_.Process(wet, mode);
    }, [&]()
    {
        detector_.Reset();
        saturation_.Reset();
        output_.Reset();
    });
}

} // namespace consolidator::audio::devices

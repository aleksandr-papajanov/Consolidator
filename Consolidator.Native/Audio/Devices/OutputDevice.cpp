#include "OutputDevice.h"

namespace consolidator::audio::devices
{

void OutputDevice::Prepare(const ProcessingContext& context)
{
    gain_.Prepare(context);
    limiter_.Prepare(context);
    transition_.Prepare(context);
}

void OutputDevice::Reset() noexcept
{
    gain_.Reset();
    limiter_.Reset();
    transition_.Reset();
}

void OutputDevice::SetParameters(const OutputParameters& parameters) noexcept
{
    gain_.SetParameters(parameters.gain);
    limiter_.SetControls(parameters.ceilingDb, true);
    transition_.SetTarget(parameters.bypass ? 0.0 : 1.0);
    bypass_ = parameters.bypass;
}

void OutputDevice::Process(AudioBuffer audio, ChannelMode mode) noexcept
{
    transition_.Process(audio, mode, [&](AudioBuffer wet)
    {
        gain_.Process(wet, mode);
        limiter_.Process(wet, mode);
    }, [&]()
    {
        gain_.Reset();
        limiter_.Reset();
    });
}

GainStageTelemetry OutputDevice::Telemetry() const noexcept
{
    return gain_.Telemetry();
}

double OutputDevice::LimiterReductionDb() const noexcept
{
    return bypass_ ? 0.0 : limiter_.ReductionDb();
}

} // namespace consolidator::audio::devices

#include "GainStage.h"

namespace consolidator::audio::devices
{

void GainStage::Prepare(const ProcessingContext& context)
{
    detector_.Prepare(context);
    leveler_.Prepare(context);
    meter_.Prepare(context);
}

void GainStage::Reset() noexcept
{
    detector_.Reset();
    leveler_.Reset();
    meter_.Reset();
}

void GainStage::SetParameters(const GainStageParameters& parameters) noexcept
{
    detector_.SetParameters(parameters.detector);
    leveler_.SetControls(parameters.levelDb, parameters.targetDb, parameters.leveler);
}

void GainStage::Process(AudioBuffer audio, ChannelMode mode) noexcept
{
    const auto detector = detector_.Capture(audio, mode);
    leveler_.Process(audio, detector, mode);
    // Target displays the audible post-gain RMS in manual mode. Detector EQ is control-only.
    meter_.Process(audio, mode);
}

GainStageTelemetry GainStage::Telemetry() const noexcept
{
    return { leveler_.SlowGainDb(), leveler_.FastCorrectionDb(), meter_.LevelDb() };
}

} // namespace consolidator::audio::devices

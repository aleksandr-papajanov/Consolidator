#pragma once

#include "GainStage.h"
#include "WetTransition.h"
#include "../Primitives/Width.h"

namespace consolidator::audio::devices
{

struct InputParameters
{
    GainStageParameters gain;
    double width{ 1.0 };
    bool bypass{};
};

class InputDevice final
{
public:
    void Prepare(const ProcessingContext& context);
    void Reset() noexcept;
    void SetParameters(const InputParameters& parameters) noexcept;
    void Process(AudioBuffer audio, ChannelMode mode) noexcept;
    [[nodiscard]] GainStageTelemetry Telemetry() const noexcept;

private:
    primitives::Width width_;
    GainStage gain_;
    WetTransition transition_;
};

} // namespace consolidator::audio::devices

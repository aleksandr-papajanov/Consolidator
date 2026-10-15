#pragma once

#include "GainStage.h"
#include "WetTransition.h"
#include "../Algorithms/Limiter/LimiterProcessor.h"

namespace consolidator::audio::devices
{

struct OutputParameters
{
    GainStageParameters gain;
    double ceilingDb{};
    bool bypass{};
};

class OutputDevice final
{
public:
    void Prepare(const ProcessingContext& context);
    void Reset() noexcept;
    void SetParameters(const OutputParameters& parameters) noexcept;
    void Process(AudioBuffer audio, ChannelMode mode) noexcept;
    [[nodiscard]] GainStageTelemetry Telemetry() const noexcept;
    [[nodiscard]] double LimiterReductionDb() const noexcept;

private:
    GainStage gain_;
    algorithms::LimiterProcessor limiter_;
    WetTransition transition_;
    bool bypass_{};
};

} // namespace consolidator::audio::devices

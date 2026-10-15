#pragma once

#include <array>

#include "DetectorEqualizer.h"
#include "../Algorithms/Leveler/LevelerProcessor.h"
#include "../Primitives/RmsMeter.h"

namespace consolidator::audio::devices
{

struct GainStageParameters
{
    double levelDb{};
    double targetDb{};
    bool leveler{};
    std::array<FilterParameters, 2> detector{};
};

struct GainStageTelemetry
{
    double levelDb{};
    double fastCorrectionDb{};
    double rmsDb{ -120.0 };
};

// Input and Output instantiate this same algorithm with independent state.
class GainStage final
{
public:
    void Prepare(const ProcessingContext& context);
    void Reset() noexcept;
    void SetParameters(const GainStageParameters& parameters) noexcept;
    void Process(AudioBuffer audio, ChannelMode mode) noexcept;
    [[nodiscard]] GainStageTelemetry Telemetry() const noexcept;

private:
    DetectorEqualizer detector_;
    algorithms::LevelerProcessor leveler_;
    primitives::RmsMeter meter_;
};

} // namespace consolidator::audio::devices

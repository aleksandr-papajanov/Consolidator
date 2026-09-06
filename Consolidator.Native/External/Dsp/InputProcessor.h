#pragma once

#include "DetectorProcessor.h"
#include "DspParameterRamp.h"
#include "DspProcessor.h"
#include "LevelerProcessor.h"

namespace consolidator::max::dsp
{

class InputProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

    [[nodiscard]] bool RequiresProcessing() const noexcept;
    [[nodiscard]] bool IsLevelerEnabled() const noexcept;
    [[nodiscard]] float SlowInputLevelDb() const noexcept;
    [[nodiscard]] float EffectiveInputLevelDb() const noexcept;
    [[nodiscard]] float FastInputLevelDb() const noexcept;

private:
    void ProcessWidth(DspProcessingFrame& frame) noexcept;

    DetectorProcessor levelerDetector_{ 0 };
    LevelerProcessor leveler_;
    DspParameterRamp width_{ 1.0 };
};

} // namespace consolidator::max::dsp

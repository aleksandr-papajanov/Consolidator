#pragma once

#include "DspParameterRamp.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

class CompressionProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    DspParameterRamp amount_;
    DspParameterRamp activeMix_;
    DspParameterRamp attackSeconds_{ 0.025 };
    DspParameterRamp releaseSeconds_{ 0.060 };
    double sampleRate_{ 48000.0 };
    double detectorEnergy_{};
    double referenceEnergy_{};
    double inputEnergy_{};
    double outputEnergy_{};
    double reductionDb_{};
    double makeupDb_{};
    double reductionDuration_{};
    double measuredReductionDb_{};
    double reductionCorrectionDb_{};
    double detectorCoefficient_{};
    double referenceCoefficient_{};
    double meterCoefficient_{};
    double makeupCoefficient_{};
    bool referenceInitialized_{};
};

} // namespace consolidator::max::dsp

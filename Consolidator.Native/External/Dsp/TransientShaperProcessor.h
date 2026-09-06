#pragma once

#include "DspAutoGain.h"
#include "DspParameterRamp.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

class TransientShaperProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    DspParameterRamp attack_;
    DspParameterRamp sustain_;
    DspParameterRamp activeMix_;
    double fastEnvelope_{};
    double slowEnvelope_{};
    double sustainEnvelope_{};
    double sustainCoefficient_{};
    DspAutoGain autoGain_;
    double detectorEnergy_{};
    double detectorCoefficient_{};
    bool detectorGateOpen_{};
    double gainDb_{};
    double fastAttackCoefficient_{};
    double slowAttackCoefficient_{};
    double gainAttackCoefficient_{};
    double gainReleaseCoefficient_{};
};

} // namespace consolidator::max::dsp

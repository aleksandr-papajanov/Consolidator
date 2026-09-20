#pragma once

#include "DspAutoGain.h"
#include "DspParameterRamp.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

struct TransientShaperTuning
{
    double detectorSeconds{ 0.005 };
    double fastEnvelopeSeconds{ 0.002 };
    double slowEnvelopeSeconds{ 0.025 };
    double sustainEnvelopeSeconds{ 0.120 };
    double gainAttackSeconds{ 0.001 };
    double gainReleaseSeconds{ 0.050 };
    double attackScale{ 1.25 };
    double sustainScale{ 1.5 };
    double maximumAttackDb{ 7.5 };
    double maximumSustainDb{ 9.0 };
    double minimumDetectorLevel{ 3.1622776601683795e-5 };
    DspAutoGainTuning autoGain;
};

class TransientShaperProcessor final : public DspProcessor
{
public:
    void SetTuningBeforePrepare(const TransientShaperTuning& tuning) noexcept;
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    TransientShaperTuning tuning_;
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

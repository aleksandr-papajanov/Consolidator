#pragma once

#include <array>
#include <cstddef>

#include "DspParameterRamp.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

struct TransientShaperTuning
{
    double sidechainHz{ 20 };
    double fastAttackSeconds{ 0.0034477633992907584 };
    double fastReleaseSeconds{ 0.016644667100611195 };
    double slowAttackSeconds{ 0.002835169247362377 };
    double slowReleaseSeconds{ 0.567710585907637 };
    double contextSeconds{ 0.008885734518090072 };
    double riseDb{ 12.564163257544818 };
    double attackReleaseSeconds{ 0.01351927729339474 };
    double attackAdditionReleaseSeconds{ 0.06 };
    double attackAdditionMix{ 0.1 };
    double maskCloseSeconds{ 0.000024749356830366314 };
    double sustainOpenSeconds{ 0.0011378798407084899 };
    double peakReleaseSeconds{ 0.27062024726861245 };
    double activityExponent{ 0.06254245501785538 };
    double tailBoost{ 0.885334151720199 };
    double tailExponent{ 0.4259033384869872 };
    double baseGainDb{ -4.55 };
    double steadyReturn{ 0.12244010826368798 };
    double attackBoost{ 4.736178853237579 };
    double strengthRangeDb{ 116.89195607518414 };
    double initialSlowLevelDb{ -20 };
};

class TransientShaperProcessor final : public DspProcessor
{
public:
    void SetTuningBeforePrepare(const TransientShaperTuning& tuning) noexcept;
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void SetAmounts(double attack, double sustain) noexcept;
    void SetOutputGainDb(double gainDb) noexcept;
    void Process(DspProcessingFrame& frame) noexcept override;
    void Process(DspAudioBuffer audio, DspChannelMode mode = DspChannelMode::Stereo) noexcept;

private:
    double Filter(double input, std::size_t channel) noexcept;

    TransientShaperTuning tuning_;
    DspParameterRamp attackAmount_;
    DspParameterRamp sustainAmount_;
    DspParameterRamp outputGain_{ 1.0 };
    double outputHighpassCoefficient_{};
    std::array<std::array<double, 1>, 2> outputHighpassStates_{};
    std::array<std::array<double, 2>, 2> filterStates_{};
    double b0_{};
    double b1_{};
    double b2_{};
    double a1_{};
    double a2_{};
    double fastAttackCoefficient_{};
    double fastReleaseCoefficient_{};
    double slowAttackCoefficient_{};
    double slowReleaseCoefficient_{};
    double onsetAttackCoefficient_{};
    double onsetReleaseCoefficient_{};
    double contextCoefficient_{};
    double attackOpenCoefficient_{};
    double attackReleaseCoefficient_{};
    double attackAdditionDecay_{};
    double maskCloseCoefficient_{};
    double sustainOpenCoefficient_{};
    double peakDecay_{};
    double peakDecayDb_{};
    double riseNormalization_{};
    double baseGain_{};
    double initialSlowPower_{};
    double fastPower_{};
    double slowPower_{};
    double onsetPower_{};
    double contextPower_{};
    double peak_{};
    double peakDb_{ -240.0 };
    double attackMask_{};
    double extendedAttackMask_{};
    double sustainMask_{};
};

} // namespace consolidator::max::dsp

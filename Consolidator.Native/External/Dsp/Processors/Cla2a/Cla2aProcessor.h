#pragma once

#include <array>
#include <cstddef>

#include "../../DspBiquad.h"
#include "../../DspParameterRamp.h"
#include "../../DspProcessingContext.h"

namespace consolidator::max::dsp::processors
{

// Reduction moves the compression threshold; Output is an independent audio amplitude.
struct Cla2aControls
{
    double reduction{ 0.598 };
    double outputGain{ 0.02985085 };
};

// Fixed optical model configuration, compiled once and shared by processor instances.
struct Cla2aConfig
{
    double thresholdAt40Db{ -2.70 };
    double lowReductionSlopeDbPerUnit{ 1.69 };
    std::array<double, 6> thresholdSlopesDbPerUnit{ 1.9555, 0.6224, 0.6157, 0.4709, 0.3265, 0.3108 };
    double outputCalibrationDb{ 39.0 };
    double kneeDb{ 6.81 };
    double compressionSlope{ 0.768 };
    double deepReductionStartDb{ 25.0 };
    double deepReductionSpanDb{ 3.0 };
    double deepReductionStrength{ 0.135 };
    double detectorAttackSeconds{ 0.0001 };
    double detectorReleaseSeconds{ 0.0003 };
    // Saturating charge drive bounds the attack speed; recovery follows memory and remaining target.
    double chargeSaturationDb{ 22.0 };
    double fastChargeBoostDepthDb{ 14.0 };
    double fastChargeFlattenDepthDb{ 17.7 };
    double fastChargeScale{ 0.00383 };
    double slowChargeScale{ 0.0513 };
    double fastRecoverySeconds{ 0.308 };
    double slowRecoverySeconds{ 3.35 };
    double recoveryDepthDb{ 10.0 };
    double recoveryTargetScaleDb{ 4.33 };
    double highPassHz{ 4.3 };
    double highShelfHz{ 6000 };
    double highShelfQ{ 0.7 };
    double highShelfDb{ -0.6 };
    double slowFraction{ 0.1903 };
};

// Optical approximation with a variable threshold and two reduction memories.
class Cla2aProcessor final
{
public:
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetControls(double reduction, double outputGain) noexcept;
    void Process(DspAudioBuffer audio, DspChannelMode mode = DspChannelMode::Stereo) noexcept;

private:
    void PrepareDetector(double sampleRate) noexcept;
    void UpdateReductionCurve(double reduction) noexcept;
    void PrepareColourFilters(const DspProcessingContext& context) noexcept;
    [[nodiscard]] double CalculateTargetReductionDb() const noexcept;
    void UpdateDetector(const std::array<double*, 2>& channels, std::size_t index) noexcept;
    [[nodiscard]] double UpdateOpticalGain(double targetDb) noexcept;
    [[nodiscard]] double ProcessHighPassSample(double input, std::size_t channel) noexcept;

    inline static constexpr Cla2aConfig kConfig{};
    DspParameterRamp reduction_{ Cla2aControls{}.reduction };
    DspParameterRamp output_{ Cla2aControls{}.outputGain };
    double cachedReduction_{ -1.0 };
    double thresholdDb_{};
    double outputCalibration_{};
    double detectorAttackCoefficient_{};
    double detectorReleaseCoefficient_{};
    double inverseSampleRate_{};

    // Shared detector power precedes two dB reduction histories; filters remain per channel.
    double detectorPower_{};
    double fastReductionDb_{};
    double slowReductionDb_{};
    double compressionStartPower_{};
    double highPassFeedforward_{};
    double highPassFeedback_{};
    std::array<std::array<double, 2>, 2> highPassStates_{};
    DspBiquadCascade highShelfFilter_;
};

} // namespace consolidator::max::dsp::processors

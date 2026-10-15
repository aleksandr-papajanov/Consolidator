#pragma once

#include <array>
#include <cstddef>

#include "../CompressorControlMode.h"
#include "../../DspBiquad.h"
#include "../../DspParameterRamp.h"
#include "../../DspProcessingContext.h"

namespace consolidator::max::dsp::processors
{

// Shared reference taper for Native control laws and VST3 numeric entry.
struct Cla2aControlScale
{
    double pivot{};
    double lowExponent{};
    double highExponent{};

    [[nodiscard]] double ToDisplayValue(double normalized) const noexcept;
    [[nodiscard]] double ToNormalizedValue(double displayed) const noexcept;
};

inline constexpr Cla2aControlScale kCla2aGainScale{ 0.35, 6.507, 4.3812 };
inline constexpr Cla2aControlScale kCla2aPeakReductionScale{ 0.4, 5.994, 4.6213 };

// Normalized reference knobs; Gain remains outside the detector.
struct Cla2aEmulationControls
{
    double gain{ 0.535 };
    double peakReduction{ 0.598 };
};

struct Cla2aControls : CompressorControls
{
    Cla2aEmulationControls emulation;
};

// Fixed optical model configuration, compiled once and shared by processor instances.
struct Cla2aConfig
{
    double thresholdAt40Db{ -2.70 };
    double lowReductionSlopeDbPerUnit{ 1.69 };
    std::array<double, 6> thresholdSlopesDbPerUnit{ 1.9555, 0.6224, 0.6157, 0.4709, 0.3265, 0.3108 };
    double gainFloorDb{ -51.35 };
    // Measured dB slopes in successive ten-unit regions, not interpolation anchors.
    std::array<double, 10> gainSlopesDbPerUnit{
        1.6400, 1.6903, 1.5516, 1.1034, 0.8441, 0.7837, 0.5018, 0.4010, 0.1503, 0.3506
    };
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
    explicit Cla2aProcessor(CompressorControlMode controlMode = CompressorControlMode::Normal) noexcept;
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetControls(const Cla2aControls& controls) noexcept;
    void Process(DspAudioBuffer audio, DspChannelMode mode = DspChannelMode::Stereo) noexcept;

private:
    void PrepareDetector(double sampleRate) noexcept;
    void UpdateReductionCurve(double peakReduction) noexcept;
    void UpdateOutputGain(double gain) noexcept;
    void PrepareColourFilters(const DspProcessingContext& context) noexcept;
    [[nodiscard]] double CalculateTargetReductionDb() const noexcept;
    void UpdateDetector(const std::array<double*, 2>& channels, std::size_t index) noexcept;
    [[nodiscard]] double UpdateOpticalGain(double targetDb) noexcept;
    [[nodiscard]] double ProcessHighPassSample(double input, std::size_t channel) noexcept;

    inline static constexpr Cla2aConfig kConfig{};
    const CompressorControlMode controlMode_;
    DspParameterRamp peakReduction_;
    DspParameterRamp gain_;
    double cachedPeakReduction_{ -1.0 };
    double cachedGain_{ -1.0 };
    double thresholdDb_{};
    double outputGain_{};
    double makeupGain_{ 1.0 };
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

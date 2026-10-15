#pragma once

#include <array>
#include <cstddef>

#include "../CompressorControlMode.h"
#include "../../DspParameterRamp.h"
#include "../../DspProcessingContext.h"

namespace consolidator::max::dsp::processors
{

// Reference Input/Output attenuators, Attack/Release 1..7 (slow to fast), Ratio 4:1 to ALL.
struct Cla76EmulationControls
{
    double inputGain{ 0.03162277660168379 };
    double outputGain{ 0.12589254117941673 };
    double attack{ 3.0 };
    double release{ 4.0 };
    double ratio{};
};

struct Cla76Controls : CompressorControls
{
    double attack{ 2.0 / 3.0 };
    double release{ 0.5 };
    double ratio{ kNormalRatioDefault };
    Cla76EmulationControls emulation;
};

struct Cla76CompressionConfig
{
    double thresholdDb{ -25.7 };
    double kneeDb{ 6.0 };
    double compressionSlope{ 0.75 };
    // Log seconds: c0 + c1*a + c2*abs(a).
    std::array<double, 3> attackLaw{ -5.125, -1.292, -0.729 };
    // Ascending polynomial coefficients in r=(Release-4)/3, log seconds.
    std::array<double, 6> releaseLaw{
        -0.173, -1.306, -0.732, -0.487, 0.0, 0.284 };
    double outputOffsetDb{};
    double thresholdAttackSpanDb{};
};

struct Cla76Config
{
    Cla76CompressionConfig normal;
    Cla76CompressionConfig all{
        -25.84, 6.0, 0.9,
        { -6.918, -1.498, -0.812 },
        { -1.899, -1.597, -1.284, -0.923, 0.101, 0.489 },
        0.65, 0.46 };
    std::array<double, 3> allCarrierAttackLaw{ -5.999, -1.04, -0.575 };
    double allReleaseCurrentRatio{ 1.5 };
    double allFastReleaseThresholdAttackSpanDb{ 1.0 };
    double allFastReleaseSlopeAttackSpan{ 0.020 };
    // Log-domain quadratic surfaces: 1, a, r, a*a, a*r, r*r; a/r=(control-4)/3.
    std::array<double, 6> allMemoryLimitLaw{
        1.59, -0.24, -0.61, -0.22, 0.0, -0.31 };
    std::array<double, 6> allMemoryTimeLaw{
        -0.07, 0.284, -0.325, 0.182, -0.231, -0.104 };
    // Ascending polynomial coefficients in r=(Release-4)/3.
    std::array<double, 5> allThresholdLaw{ 0.47, 1.355, 0.84, -1.395, -1.89 };
    std::array<double, 5> allSlopeLaw{ 0.006, -0.004, -0.037, -0.021, 0.006 };
    double colourPoleHz{ 14800 };
    double colourFeedthrough{ 0.652 };
    double inputCalibrationDb{ 30.0 };
    double outputCalibrationDb{ 27.2 };
};

// Bluey core: Normal 1:1-to-ALL, retained reference 4:1-to-ALL in Emulation.
class Cla76Processor final
{
public:
    explicit Cla76Processor(CompressorControlMode controlMode = CompressorControlMode::Normal) noexcept;
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetControls(const Cla76Controls& controls) noexcept;
    void Process(DspAudioBuffer audio, DspChannelMode mode = DspChannelMode::Stereo) noexcept;

private:
    void UpdateMode(double ratio) noexcept;
    void UpdateControlCoefficients(
        double attack, double release, double ratio, double reduction, double ratioControl) noexcept;
    [[nodiscard]] double CalculateTargetReductionDb() const noexcept;
    void UpdateDetector(double detectorAmplitude, double ratio) noexcept;
    [[nodiscard]] double UpdateMemoryReductionDb(double targetDb, double ratio) noexcept;
    [[nodiscard]] double ProcessColourSample(double input, std::size_t channel) noexcept;

    inline static constexpr Cla76Config kConfig{};
    const CompressorControlMode controlMode_;
    Cla76CompressionConfig activeMode_;
    DspParameterRamp reduction_{ Cla76Controls{}.reduction };
    double cachedReduction_{ -1.0 };
    DspParameterRamp inputGain_;
    DspParameterRamp outputGain_;
    DspParameterRamp attack_{ Cla76EmulationControls{}.attack };
    DspParameterRamp release_{ Cla76EmulationControls{}.release };
    DspParameterRamp ratio_;

    // Curve and time coefficients are cached until their live controls change.
    double sampleRate_{};
    double inputCalibration_{};
    double outputCalibration_{};
    double modeOutputGain_{ 1.0 };
    double makeupGain_{ 1.0 };
    double attackCoefficient_{};
    double releaseCoefficient_{};
    double cachedNominalInput_{ -1.0 };
    double cachedNominalOutput_{ -1.0 };
    double cachedAttack_{ -1.0 };
    double cachedRelease_{ -1.0 };
    double cachedRatio_{ -1.0 };
    double cachedRatioControl_{ -1.0 };

    // One linked detector and a warm ALL memory serve every Ratio setting.
    double detectorPower_{};
    double allMemoryReductionDb_{};
    double allMemoryLimitDb_{};
    double allMemoryCoefficient_{};
    double allReleaseCurrent_{};
    double activeThresholdDb_{};
    double compressionStartPower_{};
    double activeCompressionSlope_{};
    double colourPole_{};
    double colourNumerator_{};
    std::array<double, 2> colourPoleStates_{};
};

} // namespace consolidator::max::dsp::processors

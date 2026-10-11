#pragma once

#include <array>
#include "DspParameterRamp.h"
#include "DspProcessingContext.h"

namespace consolidator::max::dsp
{

struct Cla76Controls
{
    double inputGain{ 0.03 };
    double outputGain{ 0.12 };
    double attack{ 0.66 };
    double release{ 0.5 };
    double ratio{};
};

struct Cla76CompressionTuning
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

struct Cla76Tuning
{
    Cla76CompressionTuning normal;
    Cla76CompressionTuning all{
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

// Shared Bluey core with continuous 4:1-to-ALL controls.
class Cla76Processor final
{
public:
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetControls(
        double inputGain, double outputGain, double attack, double release, double ratio) noexcept;
    void Process(DspAudioBuffer audio, DspChannelMode mode = DspChannelMode::Stereo) noexcept;

private:
    const Cla76Tuning tuning_;
    Cla76CompressionTuning activeMode_;
    DspParameterRamp inputGain_{ Cla76Controls{}.inputGain };
    DspParameterRamp outputGain_{ Cla76Controls{}.outputGain };
    DspParameterRamp attack_{ 7.0 - 6.0 * Cla76Controls{}.attack };
    DspParameterRamp release_{ 7.0 - 6.0 * Cla76Controls{}.release };
    DspParameterRamp ratio_{ Cla76Controls{}.ratio };
    double sampleRate_{};
    double inputCalibration_{};
    double outputCalibration_{};
    double modeOutputGain_{ 1.0 };
    double attackCoefficient_{};
    double releaseCoefficient_{};
    double cachedAttack_{ -1.0 };
    double cachedRelease_{ -1.0 };
    double cachedRatio_{ -1.0 };
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

} // namespace consolidator::max::dsp

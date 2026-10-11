#pragma once

#include <array>
#include <cstddef>

#include "DspBiquad.h"
#include "DspParameterRamp.h"
#include "DspProcessingContext.h"

namespace consolidator::max::dsp
{

struct Cla76Controls
{
    double inputGain{ 0.03162277660168379 };
    double outputGain{ 0.12589254117941673 };
    double attack{ 2.0 / 3.0 };
    double release{ 0.5 };
    double ratio{};
};

struct Cla76CompressionTuning
{
    double thresholdDb{ -25.7 };
    double kneeDb{ 6.0 };
    double compressionSlope{ 0.75 };
    // Log seconds: c0 + c1*a + c2*(sqrt(a*a + 0.02*0.02) - 0.02).
    std::array<double, 3> attackLaw{ -5.125359509721171, -1.2918197081985978, -0.729459131745406 };
    // Ascending polynomial coefficients in r=(Release-4)/3, log seconds.
    std::array<double, 6> releaseLaw{
        -0.17332146514171948, -1.3056680359959605, -0.7315240314736943,
        -0.486904097125937, 0.00003040339794666644, 0.2841467472894941 };
    double outputOffsetDb{};
    double thresholdAttackSpanDb{};
};

struct Cla76Tuning
{
    Cla76CompressionTuning normal;
    Cla76CompressionTuning all{
        -25.83700092117243, 6.0, 0.902,
        { -6.918178716004377, -1.4976668027306437, -0.8120156932379566 },
        { -1.8989180202116605, -1.5967680072896404, -1.2838562017458446,
            -0.9225218105292984, 0.10055313825791018, 0.48935515935888774 },
        0.65, 0.45899079779061014 };
    std::array<double, 3> allCarrierAttackLaw{
        -5.9988440312123705, -1.0395796482501838, -0.5748830148800053 };
    double allReleaseCurrentRatio{ 1.5 };
    double allFastReleaseThresholdAttackSpanDb{ 1.0 };
    double allFastReleaseSlopeAttackSpan{ 0.020 };
    // Log-domain quadratic surfaces: 1, a, r, a*a, a*r, r*r; a/r=(control-4)/3.
    std::array<double, 6> allMemoryLimitLaw{
        1.5912662073743356, -0.242211326642396, -0.6096587288368607,
        -0.22482866754650016, 0.001532296633991284, -0.3105861935699487 };
    std::array<double, 6> allMemoryTimeLaw{
        -0.07037682319881366, 0.2838750608192708, -0.3250469108680383,
        0.18243866968916117, -0.23098252416309983, -0.10431072445928585 };
    // Ascending polynomial coefficients in r=(Release-4)/3.
    std::array<double, 5> allThresholdLaw{ 0.47, 1.355, 0.84, -1.395, -1.89 };
    std::array<double, 5> allSlopeLaw{ 0.00605, -0.003625, -0.037125, -0.021375, 0.006075 };
    double lowShelfHz{ 13.698060446831752 };
    double lowShelfDb{ 0.9261177935379185 };
    double bellHz{ 83.1408184281944 };
    double bellQ{ 0.5314781351325771 };
    double bellDb{ -0.20324792807856784 };
    double colourPoleHz{ 14786.662130741144 };
    double colourFeedthrough{ 0.6523724094256391 };
    double inputCalibrationDb{ 30.0 };
    double outputCalibrationDb{ 27.18 };
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
    std::array<DspBiquadCascade, 2> colourFilters_;
    double colourPole_{};
    double colourNumerator_{};
    std::array<double, 2> colourPoleStates_{};
};

} // namespace consolidator::max::dsp

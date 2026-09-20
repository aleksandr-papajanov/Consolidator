#pragma once

#include <array>
#include <vector>

#include "DspBiquad.h"
#include "DspParameterRamp.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

struct PolishThickTuning
{
    double bodyPeakFrequency{ 81.038 };
    double bodyPeakGainDb{ 14.6765 };
    double bodyPeakQ{ 0.724975 };
    double subNotchFrequency{ 34.1538 };
    double subNotchGainDb{ -9.42724 };
    double subNotchQ{ 1.66623 };
    double positiveShoulderFrequency{ 19.0 };
    double positiveShoulderGainDb{ 9.57224 };
    double positiveShoulderQ{ 1.4 };
    double positivePeakFrequency{ 52.0 };
    double positivePeakGainDb{ -16.6703 };
    double positivePeakQ{ 1.15 };
    std::array<double, 7> bodyMixAnchors{
        1.00145, 0.645654, 0.468731, 0.390001,
        0.318927, 0.212151, 0.0683867
    };
    double sourceCutoffHz{ 60.0 };
    double harmonicHighPassHz{ 60.0 };
    double harmonicLowPassHz{ 18000.0 };
    double feedbackLoopGain{ 0.78 };
    double evenHarmonicWeight{ 0.03 };
    double denseHarmonicGain{ 0.12 };
    double denseHarmonicDecay{ 0.97 };
    double lowLevelDrive{ 0.0184 };
    double richLevelDrive{ 0.9816 };
    double subHarmonicCutoffHz{ 120.0 };
    double subHarmonicDrive{ 0.1 };
    double transientCrestThreshold{ 1.25 };
    double transientMinimumGain{ 0.35 };
    double transientBodyControl{ 0.35 };
};

class PolishThickProcessor final : public DspProcessor
{
public:
    void SetTuningBeforePrepare(const PolishThickTuning& tuning) noexcept;
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    struct Channel
    {
        std::array<double, 1> rumble{};
        std::array<double, 2> source{};
        std::array<double, 2> highPass{};
        std::array<double, 2> lowPass{};
        std::array<double, 2> lowLevelHighPass{};
        std::array<double, 2> lowLevelLowPass{};
        std::array<double, 2> subHarmonicSource{};
        std::array<double, 2> subHarmonicHighPass{};
        std::array<double, 2> subHarmonicLowPass{};
        double positiveFeedback{};
        double negativeFeedback{};
    };

    DspParameterRamp amount_;
    DspBiquadCascade bodyPeak_;
    DspBiquadCascade subNotch_;
    DspBiquadCascade positiveSubShoulder_;
    DspBiquadCascade positiveBodyPeak_;
    std::vector<double> bodyLeft_;
    std::vector<double> bodyRight_;
    std::vector<double> positiveBodyLeft_;
    std::vector<double> positiveBodyRight_;
    std::array<Channel, 2> channels_{};
    double rumbleCoefficient_{};
    double sourceCoefficient_{};
    double highPassCoefficient_{};
    double lowPassCoefficient_{};
    double subHarmonicSourceCoefficient_{};
    double energyCoefficient_{};
    double energy_{};
    double mainEnergy_{};
    double energyWeight_{};
    double wetEnergy_{};
    double fastWetEnergy_{};
    double fastEnergyWeight_{};
    double fastEnergyCoefficient_{};
    double gainAttackCoefficient_{};
    double gainReleaseCoefficient_{};
    double harmonicGain_{ 1.0 };
    double sourcePeak_{};
    double fullBandPeak_{};
    double peakAttackCoefficient_{};
    double peakReleaseCoefficient_{};
    double transientPeakReleaseCoefficient_{};
    double transientGainAttackCoefficient_{};
    double transientGainReleaseCoefficient_{};
    double transientPeak_{};
    double transientGain_{ 1.0 };
    double bodyLevelMix_{ 1.00145 };
    double positiveLevelMix_{};
    PolishThickTuning tuning_{};
};

} // namespace consolidator::max::dsp

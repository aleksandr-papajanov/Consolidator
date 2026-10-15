#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "../../Primitives/Filters/BiquadCascade.h"
#include "../../Primitives/ParameterRamp.h"
#include "../../Core/AudioBuffer.h"

namespace consolidator::audio::algorithms
{

struct PolishThickConfig
{
    double bodyPeakFrequency{ 81.0 };
    double bodyPeakGainDb{ 14.68 };
    double bodyPeakQ{ 0.725 };
    double subNotchFrequency{ 34.15 };
    double subNotchGainDb{ -9.43 };
    double subNotchQ{ 1.67 };
    double positiveShoulderFrequency{ 19.0 };
    double positiveShoulderGainDb{ 9.57 };
    double positiveShoulderQ{ 1.4 };
    double positivePeakFrequency{ 52.0 };
    double positivePeakGainDb{ -16.67 };
    double positivePeakQ{ 1.15 };
    // Rational numerator coefficients in sqrt(input amplitude), then denominator slope.
    std::array<double, 4> bodyMixLaw{ 1.599, -2.071, 2.404, 0.87 };
    double bodyMixFloor{ 0.0684 };
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

// Linked bass analysis drives parallel body, rich, low-level and sub additions.
class PolishThickProcessor final
{
public:
    void Prepare(const ProcessingContext& context);
    void Reset() noexcept;
    void SetAmount(double value) noexcept;
    void Process(AudioBuffer buffer, ChannelMode mode) noexcept;

private:
    // Per-sample linked analysis; no borrowed audio pointers or dynamic storage.
    struct SourceAnalysis
    {
        std::array<double, 2> bass;
        double bassMeanSquare;
        double fullBandMeanSquare;
        double inverseStartupWeight;
        double bassPeak;
        double bassPresence;
        double fullBandPower;
    };

    struct SampleControls
    {
        double amount;
        double scale;
        double inverseScale;
        double loopGain;
        double lowLevelDrive;
        double richLevelDrive;
        double intensity;
    };

    struct HarmonicBands
    {
        std::array<double, 2> rich;
        std::array<double, 2> lowLevel;
        std::array<double, 2> sub;
        double richPower;
    };

    void PrepareBodyFilters(const ProcessingContext& context);
    void PrepareSourceAndDynamics(double sampleRate) noexcept;
    void ProcessBodyPaths(const std::array<double*, 2>& audio, std::size_t frameCount, ChannelMode mode) noexcept;
    [[nodiscard]] SourceAnalysis AnalyseSource(const std::array<double*, 2>& audio, std::size_t index) noexcept;
    [[nodiscard]] SampleControls UpdateSourceControls(const SourceAnalysis& source) noexcept;
    void UpdateCrestGain(double peak, double bassReferenceLevel) noexcept;
    void UpdateBodyLevelMix(double bodyControlLevel) noexcept;
    [[nodiscard]] HarmonicBands GenerateHarmonicBands(
        const std::array<double*, 2>& audio, const SourceAnalysis& source, const SampleControls& controls) noexcept;
    void UpdateWetGain(const SourceAnalysis& source, const SampleControls& controls, double wetEnergy) noexcept;
    void MixOutputSample(const std::array<double*, 2>& audio, std::size_t index,
        const SampleControls& controls, const HarmonicBands& bands) noexcept;

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

    // Body branches are prepared before the audio callback; histories are per channel.
    primitives::ParameterRamp amount_;
    primitives::filters::BiquadCascade bodyPeak_;
    primitives::filters::BiquadCascade subNotch_;
    primitives::filters::BiquadCascade positiveSubShoulder_;
    primitives::filters::BiquadCascade positiveBodyPeak_;
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
    // Startup-weighted source/wet power prevents long RMS windows from suppressing cold attacks.
    double sourcePower_{};
    double fullBandPower_{};
    double sourceStartupWeight_{};
    double richWetPower_{};
    double fastRichWetPower_{};
    double wetStartupWeight_{};
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
    double denseTailWeight_{};
    double bodyLevelMix_{ 1.0 };
    double positiveLevelMix_{};
    inline static constexpr PolishThickConfig kConfig{};
};

} // namespace consolidator::audio::algorithms

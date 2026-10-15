#pragma once

#include <array>
#include <cstddef>

#include "../CompressorControlMode.h"
#include "../../DspBiquad.h"
#include "../../DspParameterRamp.h"
#include "../../DspProcessingContext.h"

namespace consolidator::max::dsp::processors
{

// Reference-normalized Gain/Peak Reduction; current optical DSP and colour remain unchanged.
struct Cla3aEmulationControls
{
    double gain{ 0.55 };
    double peakReduction{ 0.4 };
};

struct Cla3aControls : CompressorControls
{
    Cla3aEmulationControls emulation;
};

// Fixed optical model configuration, compiled once and shared by processor instances.
struct Cla3aConfig
{
    double gainUnityPosition{ 0.408 };
    double gainSpanDb{ 50.0 };
    // Polynomial in (Peak Reduction - 0.6)/0.4, recovered from the final simplified model.
    std::array<double, 4> thresholdLaw{ -27.37, -22.94, 14.42, -5.13 };
    double kneeDb{ 12.5 };
    double compressionSlope{ 0.735 };
    double detectorAttackSeconds{ 0.0001 };
    double detectorReleaseSeconds{ 0.0017 };
    double fastAttackSeconds{ 0.005 };
    double slowAttackSeconds{ 0.075 };
    double fastReleaseSeconds{ 0.105 };
    double slowReleaseSeconds{ 0.8 };
    double releaseDriveDb{ 5.8 };
    double releaseFloorSeconds{ 0.00006 };
    double detectorShelfHz{ 740 };
    double detectorLowDb{ -1.8 };
    double highPassHz{ 8.8 };
    double highPassQ{ 0.62 };
    double highShelfHz{ 14500 };
    double highShelfQ{ 0.64 };
    double highShelfDb{ 2.2 };
    double slowFraction{ 0.18 };
};

// Variable-threshold optical approximation with fast and slow gain-reduction memories.
class Cla3aProcessor final
{
public:
    explicit Cla3aProcessor(CompressorControlMode controlMode = CompressorControlMode::Normal) noexcept;
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetControls(const Cla3aControls& controls) noexcept;
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

    inline static constexpr Cla3aConfig kConfig{};
    const CompressorControlMode controlMode_;
    DspParameterRamp peakReduction_;
    DspParameterRamp gain_;
    double detectorAttackCoefficient_{};
    double detectorReleaseCoefficient_{};
    double fastAttackCoefficient_{};
    double slowAttackCoefficient_{};
    double inverseSampleRate_{};
    double fastZeroTargetCoefficient_{};
    double slowZeroTargetCoefficient_{};

    // Shared detector power precedes two dB reduction histories; filters remain per channel.
    double detectorPower_{};
    double detectorShelfCoefficient_{};
    double detectorLowLoss_{};
    std::array<std::array<double, 1>, 2> detectorShelfStates_{};
    double fastReductionDb_{};
    double slowReductionDb_{};
    double cachedPeakReduction_{ -1.0 };
    double cachedGain_{ -1.0 };
    double thresholdDb_{};
    double outputGain_{};
    double makeupGain_{ 1.0 };
    double compressionStartPower_{};
    std::array<double, 5> highPassCoefficients_{};
    std::array<std::array<double, 2>, 2> highPassStates_{};
    DspBiquadCascade highShelfFilter_;
};

} // namespace consolidator::max::dsp::processors

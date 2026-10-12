#pragma once

#include <array>
#include <cstddef>

#include "../../DspBiquad.h"
#include "../../DspParameterRamp.h"
#include "../../DspProcessingContext.h"

namespace consolidator::max::dsp::processors
{

// Reduction moves the compression threshold; Output is an independent audio amplitude.
struct Cla3aControls
{
    double reduction{ 0.49237375 };
    double outputGain{ 0.12 };
};

// Fixed optical model configuration, compiled once and shared by processor instances.
struct Cla3aConfig
{
    double outputCalibrationDb{ 30.0 };
    double thresholdMaximumDb{ 18.3 };
    double thresholdSpanDb{ 60.0 };
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

    inline static constexpr Cla3aConfig kConfig{};
    DspParameterRamp reduction_{ Cla3aControls{}.reduction };
    DspParameterRamp output_{ Cla3aControls{}.outputGain };
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
    double cachedReduction_{ -1.0 };
    double thresholdDb_{};
    double outputCalibration_{};
    double compressionStartPower_{};
    std::array<double, 5> highPassCoefficients_{};
    std::array<std::array<double, 2>, 2> highPassStates_{};
    DspBiquadCascade highShelfFilter_;
};

} // namespace consolidator::max::dsp::processors

#pragma once

#include <array>
#include <cstddef>

#include "../CompressorControlMode.h"
#include "../../DspParameterRamp.h"
#include "../../DspProcessingContext.h"

namespace consolidator::max::dsp::processors
{

// Physical reference units: ratio, release seconds, attack mode, threshold dBu.
struct SslChannelEmulationControls
{
    double ratio{ 1.0 };
    double releaseSeconds{ 0.4 };
    bool fastAttack{};
    double thresholdDb{};
};

struct SslChannelControls : CompressorControls
{
    double ratio{ kNormalRatioDefault };
    double release{ 0.5 };
    double attack{ 1.0 };
    SslChannelEmulationControls emulation;
};

// Function coefficients fitted to the requested manual grid, not interpolation anchors.
struct SslChannelConfig
{
    // Features: 1, q, q*q, threshold/10, log(Release/0.4), limiting amount.
    std::array<double, 6> slopeLaw{ 1.06875, -0.31955, 0.44389, 0.02217, -0.00372, 0.02604 };
    // Features: 1, q, threshold/10, Fast, log(Release/0.4).
    std::array<double, 5> onsetLaw{ -28.194, 11.498, 8.5818, -0.7319, -0.26615 };
    // Features: 1, q, q*q, q*q*q, threshold/10, Fast, logRelease, Fast*logRelease, limiting amount.
    std::array<double, 9> bendLaw{
        16.2519, 7.2544, -70.5611, 180.3548, 0.7274, -4.6080, -1.4272, 1.1856, -2.4725 };
    std::array<double, 4> makeupLaw{ 2.26795, -1.71834, 1.71929, 0.19171 };
    double makeupColdThresholdDb{ 1.5533 };
    double detectorSeconds{ 0.001 };
    double rmsToPeakDb{ 3.0103 };
    double fastAttackSeconds{ 0.0011 };
    double normalAttackSeconds{ 0.043 };
    double slowFraction{ 0.8389 };
    double fastReleaseFloorSeconds{ 0.03133 };
    double fastReleaseScale{ 0.01518 };
    double normalSlowReleaseFloorSeconds{ 0.09543 };
    double normalSlowReleaseScale{ 0.2776 };
    double normalReleaseExponent{ 1.6558 };
    double fastSlowReleaseFloorSeconds{ 0.02471 };
    double fastSlowReleaseScale{ 0.2454 };
    double fastReleaseExponent{ 1.7573 };
};

// Linked compressor only; EQ, gate and channel routing are outside this processor.
class SslChannelProcessor final
{
public:
    explicit SslChannelProcessor(CompressorControlMode controlMode = CompressorControlMode::Normal) noexcept;
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetControls(const SslChannelControls& controls) noexcept;
    void Process(DspAudioBuffer audio, DspChannelMode mode = DspChannelMode::Stereo) noexcept;

private:
    void UpdateControlCoefficients(double ratio, double releaseSeconds, bool fastAttack, double thresholdDb) noexcept;
    void UpdateMakeup(double inverseRatio, double strength, double limiting, double threshold) noexcept;
    void UpdateTiming(double releaseSeconds, bool fastAttack) noexcept;
    [[nodiscard]] double UpdateReductionMemory(double state, double targetDb, double releaseRetention) const noexcept;
    [[nodiscard]] double CalculateTargetReductionDb() const noexcept;

    inline static constexpr SslChannelConfig kConfig{};
    const CompressorControlMode controlMode_;
    DspParameterRamp output_{ 1.0 };
    DspParameterRamp ratio_;
    DspParameterRamp release_{ SslChannelEmulationControls{}.releaseSeconds };
    DspParameterRamp threshold_;
    bool fastAttack_{};
    double sampleRate_{};
    double detectorCoefficient_{};
    double fastAttackCoefficient_{};
    double normalAttackCoefficient_{};
    double attackCoefficient_{};
    double fastReleaseRetention_{};
    double slowReleaseRetention_{};
    bool cachedFastAttack_{};
    double cachedRatio_{ -1.0 };
    double cachedThresholdDb_{ 100.0 };
    double cachedReleaseSeconds_{ -1.0 };
    double compressionSlope_{};
    double detectorOnsetDb_{};
    double bendDb_{};
    double compressionStartPower_{};
    double makeupGain_{ 1.0 };
    double detectorPower_{};
    double fastReductionDb_{};
    double slowReductionDb_{};
};

} // namespace consolidator::max::dsp::processors

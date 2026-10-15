#include "SslChannelProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

#include "../../Primitives/Math.h"
#include "../../Primitives/ChannelView.h"

namespace consolidator::audio::algorithms
{

namespace
{

constexpr double kDbToLogGain = std::numbers::ln10 / 20.0;

// Measured affine regions follow the reference scale, including its fine low range.
double ReferenceReleaseSeconds(double control) noexcept
{
    return 0.1 + control * 0.25 +
        std::max(control - 0.2, 0.0) * (1.0 / 3.0 - 0.25) +
        std::max(control - 0.35, 0.0) +
        std::max(control - 0.5, 0.0) * (4.0 - 4.0 / 3.0) +
        std::max(control - 0.65, 0.0) * (20.0 / 3.0 - 4.0) +
        std::max(control - 0.8, 0.0) * (10.0 - 20.0 / 3.0);
}

} // namespace

SslChannelProcessor::SslChannelProcessor(CompressorControlMode controlMode) noexcept
    : controlMode_(controlMode),
      ratio_(controlMode == CompressorControlMode::Emulation
          ? SslChannelEmulationControls{}.ratio : 1.0 + 99.0 * NormalRatioPosition(SslChannelControls{}.ratio)),
      threshold_(controlMode == CompressorControlMode::Emulation
          ? SslChannelEmulationControls{}.thresholdDb
          : kNormalThresholdMaximumDb - kNormalThresholdSpanDb * SslChannelControls{}.reduction)
{
}

void SslChannelProcessor::Prepare(const ProcessingContext& context) noexcept
{
    sampleRate_ = context.sampleRate;
    output_.Prepare(context);
    ratio_.Prepare(context);
    release_.Prepare(context);
    threshold_.Prepare(context);
    detectorCoefficient_ = primitives::TimeCoefficient(kConfig.detectorSeconds, sampleRate_);
    fastAttackCoefficient_ = primitives::TimeCoefficient(kConfig.fastAttackSeconds, sampleRate_);
    normalAttackCoefficient_ = primitives::TimeCoefficient(kConfig.normalAttackSeconds, sampleRate_);
    Reset();
}

void SslChannelProcessor::Reset() noexcept
{
    output_.Reset();
    ratio_.Reset();
    release_.Reset();
    threshold_.Reset();
    cachedRatio_ = -1.0;
    cachedThresholdDb_ = 100.0;
    cachedReleaseSeconds_ = -1.0;
    UpdateControlCoefficients(ratio_.CurrentValue(), release_.CurrentValue(), fastAttack_, threshold_.CurrentValue());
    reductionDb_ = 0.0;
    detectorPower_ = 0.0;
    fastReductionDb_ = 0.0;
    slowReductionDb_ = 0.0;
}

void SslChannelProcessor::SetControls(const SslChannelControls& controls) noexcept
{
    if (controlMode_ == CompressorControlMode::Emulation)
    {
        const auto& reference = controls.emulation;
        ratio_.SetTarget(reference.ratio);
        release_.SetTarget(reference.releaseSeconds);
        threshold_.SetTarget(reference.thresholdDb);
        fastAttack_ = reference.fastAttack;
    }
    else
    {
        ratio_.SetTarget(1.0 + 99.0 * NormalRatioPosition(controls.ratio));
        release_.SetTarget(ReferenceReleaseSeconds(controls.release));
        threshold_.SetTarget(kNormalThresholdMaximumDb - kNormalThresholdSpanDb * controls.reduction);
        output_.SetTarget(controls.outputGain);
        fastAttack_ = controls.attack < 0.5;
    }
}

void SslChannelProcessor::Process(AudioBuffer audio, DetectorBuffer detector, ChannelMode mode) noexcept
{
    reductionDb_ = 0.0;
    const auto selected = primitives::SelectChannels(audio, mode);
    const std::array<const double*, 2> detectorChannels{
        mode == ChannelMode::Right ? nullptr : detector.left,
        mode == ChannelMode::Left ? nullptr : detector.right };
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto threshold = threshold_.NextValue();
        UpdateControlCoefficients(ratio_.NextValue(), release_.NextValue(), fastAttack_, threshold);
        // The preceding detector state controls gain; there is no audio lookahead.
        const auto targetDb = controlMode_ == CompressorControlMode::Normal && threshold == kNormalThresholdMaximumDb
            ? 0.0 : CalculateTargetReductionDb();
        const auto amplitude = std::max(detectorChannels[0] != nullptr ? std::abs(detectorChannels[0][index]) : 0.0,
            detectorChannels[1] != nullptr ? std::abs(detectorChannels[1][index]) : 0.0);
        detectorPower_ += detectorCoefficient_ * (amplitude * amplitude - detectorPower_);
        fastReductionDb_ = UpdateReductionMemory(fastReductionDb_, targetDb, fastReleaseRetention_);
        slowReductionDb_ = UpdateReductionMemory(slowReductionDb_, targetDb, slowReleaseRetention_);
        const auto reductionDb = std::lerp(fastReductionDb_, slowReductionDb_, kConfig.slowFraction);
        reductionDb_ = std::max(reductionDb_, reductionDb);
        const auto gain = reductionDb == 0.0 ? makeupGain_ : makeupGain_ * std::exp(-reductionDb * kDbToLogGain);
        selected.ApplyGain(index, gain * output_.NextValue());
    }
}

void SslChannelProcessor::UpdateControlCoefficients(
    double ratio, double releaseSeconds, bool fastAttack, double thresholdDb) noexcept
{
    if (ratio == cachedRatio_ && thresholdDb == cachedThresholdDb_ &&
        releaseSeconds == cachedReleaseSeconds_ && fastAttack == cachedFastAttack_)
    {
        return;
    }
    const auto fastAmount = fastAttack ? 1.0 : 0.0;
    const auto thresholdChanged = thresholdDb != cachedThresholdDb_;
    const auto ratioChanged = ratio != cachedRatio_;
    const auto timingChanged = releaseSeconds != cachedReleaseSeconds_ || fastAttack != cachedFastAttack_;
    const auto emulation = controlMode_ == CompressorControlMode::Emulation;
    if (ratioChanged || timingChanged || (emulation && thresholdChanged))
    {
        // The endpoint is limiting; blend smoothly above the highest measured finite ratio.
        const auto limiting = std::clamp((ratio - 20.0) / 80.0, 0.0, 1.0);
        const auto inverseRatio = (1.0 - limiting) / ratio;
        const auto strength = 1.0 - inverseRatio;
        const auto threshold = emulation ? thresholdDb / 10.0 : 0.0;
        const auto logRelease = std::log(releaseSeconds / 0.4);
        const auto& slope = kConfig.slopeLaw;
        compressionSlope_ = strength * (slope[0] + inverseRatio * (slope[1] + inverseRatio * slope[2]) +
            threshold * slope[3] + logRelease * slope[4] + limiting * slope[5]);
        const auto& bend = kConfig.bendLaw;
        bendDb_ = bend[0] + inverseRatio * (bend[1] + inverseRatio * (bend[2] + inverseRatio * bend[3])) +
            threshold * bend[4] + fastAmount * bend[5] + logRelease * bend[6] +
            fastAmount * logRelease * bend[7] + limiting * bend[8];
        if (emulation)
        {
            const auto& onset = kConfig.onsetLaw;
            detectorOnsetDb_ = onset[0] + inverseRatio * onset[1] + threshold * onset[2] +
                fastAmount * onset[3] + logRelease * onset[4] - kConfig.rmsToPeakDb;
        }
    }
    if (emulation && (ratioChanged || thresholdChanged))
    {
        const auto limiting = std::clamp((ratio - 20.0) / 80.0, 0.0, 1.0);
        const auto inverseRatio = (1.0 - limiting) / ratio;
        UpdateMakeup(inverseRatio, 1.0 - inverseRatio, limiting, thresholdDb / 10.0);
    }
    if (!emulation)
    {
        detectorOnsetDb_ = thresholdDb - kConfig.rmsToPeakDb;
        makeupGain_ = CalculateMakeupGain(
            (kNormalThresholdMaximumDb - thresholdDb) / kNormalThresholdSpanDb,
            fastAttack, releaseSeconds, ratio);
    }
    const auto start = primitives::DbToGain(detectorOnsetDb_);
    compressionStartPower_ = start * start;
    if (timingChanged)
    {
        UpdateTiming(releaseSeconds, fastAttack);
    }
    cachedRatio_ = ratio;
    cachedThresholdDb_ = thresholdDb;
    cachedReleaseSeconds_ = releaseSeconds;
    cachedFastAttack_ = fastAttack;
}

void SslChannelProcessor::UpdateMakeup(
    double inverseRatio, double strength, double limiting, double threshold) noexcept
{
    const auto& makeup = kConfig.makeupLaw;
    const auto baseMakeupDb = strength * (makeup[0] + inverseRatio *
        (makeup[1] + inverseRatio * makeup[2])) + limiting * makeup[3];
    const auto makeupDb = baseMakeupDb * std::max(1.0 - threshold, 0.0) +
        kConfig.makeupColdThresholdDb * std::max(-threshold - 1.0, 0.0) * (-std::expm1(-strength / 0.1));
    makeupGain_ = primitives::DbToGain(makeupDb);
}

void SslChannelProcessor::UpdateTiming(double releaseSeconds, bool fastAttack) noexcept
{
    attackCoefficient_ = fastAttack ? fastAttackCoefficient_ : normalAttackCoefficient_;
    fastReleaseRetention_ = 1.0 - primitives::TimeCoefficient(
        kConfig.fastReleaseFloorSeconds + kConfig.fastReleaseScale * releaseSeconds, sampleRate_);
    const auto slowScale = fastAttack ? kConfig.fastSlowReleaseScale : kConfig.normalSlowReleaseScale;
    const auto slowFloor = fastAttack ? kConfig.fastSlowReleaseFloorSeconds : kConfig.normalSlowReleaseFloorSeconds;
    const auto exponent = fastAttack ? kConfig.fastReleaseExponent : kConfig.normalReleaseExponent;
    slowReleaseRetention_ = 1.0 - primitives::TimeCoefficient(
        slowFloor + slowScale * std::pow(releaseSeconds, exponent), sampleRate_);
}

double SslChannelProcessor::CalculateTargetReductionDb() const noexcept
{
    if (compressionSlope_ == 0.0 || detectorPower_ <= compressionStartPower_)
    {
        return 0.0;
    }
    const auto position = 10.0 * std::log10(detectorPower_) - detectorOnsetDb_;
    return compressionSlope_ * position * position / (position + bendDb_);
}

double SslChannelProcessor::UpdateReductionMemory(
    double state, double targetDb, double releaseRetention) const noexcept
{
    if (targetDb >= state)
    {
        return state + attackCoefficient_ * (targetDb - state);
    }
    // Release the stored charge toward zero, stopping at the current signal's reduction floor.
    return std::max(targetDb, state * releaseRetention);
}

double SslChannelProcessor::ReductionDb() const noexcept
{
    return reductionDb_;
}

// Model-specific settings compensation; independent of live detector history.
double SslChannelProcessor::CalculateMakeupGain(double reduction, bool fastAttack, double releaseSeconds, double ratio) noexcept
{
    const auto inverseRatio = 1.0 / ratio;
    const auto strength = 1.0 - inverseRatio;
    if (reduction == 0.0 || strength == 0.0)
    {
        return 1.0;
    }
    const auto attackPosition = fastAttack ? -0.5 : 0.5;
    const auto releasePosition = std::log(releaseSeconds / 0.4) / 3.0;
    const auto logSlope = 3.969 - 0.376 * attackPosition + 0.188 * releasePosition +
        0.156 * attackPosition * releasePosition - inverseRatio * (0.137 + 0.463 * inverseRatio);
    const auto slope = std::exp(logSlope) * strength;
    const auto onset = 0.417 + 0.070 * attackPosition - 0.040 * releasePosition + 0.049 * inverseRatio;
    const auto position = std::max(reduction - onset, 0.0);
    return primitives::DbToGain(slope * position * position / (position + 0.467));
}

} // namespace consolidator::audio::algorithms

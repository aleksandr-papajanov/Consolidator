#include "SslChannelProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

#include "../../DspMath.h"
#include "../../DspProcessingPrimitives.h"

namespace consolidator::max::dsp::processors
{

namespace
{

constexpr double kDbToLogGain = std::numbers::ln10 / 20.0;

} // namespace

void SslChannelProcessor::Prepare(const DspProcessingContext& context) noexcept
{
    sampleRate_ = context.sampleRate;
    ratio_.Prepare(context);
    release_.Prepare(context);
    threshold_.Prepare(context);
    detectorCoefficient_ = TimeCoefficient(kConfig.detectorSeconds, sampleRate_);
    fastAttackCoefficient_ = TimeCoefficient(kConfig.fastAttackSeconds, sampleRate_);
    normalAttackCoefficient_ = TimeCoefficient(kConfig.normalAttackSeconds, sampleRate_);
    Reset();
}

void SslChannelProcessor::Reset() noexcept
{
    ratio_.Reset();
    release_.Reset();
    threshold_.Reset();
    cachedRatio_ = -1.0;
    cachedThresholdDb_ = 100.0;
    cachedReleaseSeconds_ = -1.0;
    UpdateControlCoefficients(ratio_.CurrentValue(), release_.CurrentValue(), fastAttack_, threshold_.CurrentValue());
    detectorPower_ = 0.0;
    fastReductionDb_ = 0.0;
    slowReductionDb_ = 0.0;
}

void SslChannelProcessor::SetControls(
    double ratio, double releaseSeconds, bool fastAttack, double thresholdDb) noexcept
{
    ratio_.SetTarget(ratio);
    release_.SetTarget(releaseSeconds);
    threshold_.SetTarget(thresholdDb);
    fastAttack_ = fastAttack;
}

void SslChannelProcessor::Process(DspAudioBuffer audio, DspChannelMode mode) noexcept
{
    const auto selected = SelectChannels(audio, mode);
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        UpdateControlCoefficients(ratio_.NextValue(), release_.NextValue(), fastAttack_, threshold_.NextValue());
        // The preceding detector state controls gain; there is no audio lookahead.
        const auto targetDb = CalculateTargetReductionDb();
        const auto amplitude = selected.Peak(index);
        detectorPower_ += detectorCoefficient_ * (amplitude * amplitude - detectorPower_);
        fastReductionDb_ = UpdateReductionMemory(fastReductionDb_, targetDb, fastReleaseRetention_);
        slowReductionDb_ = UpdateReductionMemory(slowReductionDb_, targetDb, slowReleaseRetention_);
        const auto reductionDb = std::lerp(fastReductionDb_, slowReductionDb_, kConfig.slowFraction);
        const auto gain = reductionDb == 0.0 ? makeupGain_ : makeupGain_ * std::exp(-reductionDb * kDbToLogGain);
        selected.ApplyGain(index, gain);
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
    const auto makeupChanged = ratio != cachedRatio_ || thresholdDb != cachedThresholdDb_;
    const auto timingChanged = releaseSeconds != cachedReleaseSeconds_ || fastAttack != cachedFastAttack_;
    // The endpoint is limiting; blend smoothly above the highest measured finite ratio.
    const auto limiting = std::clamp((ratio - 20.0) / 80.0, 0.0, 1.0);
    const auto inverseRatio = (1.0 - limiting) / ratio;
    const auto strength = 1.0 - inverseRatio;
    const auto threshold = thresholdDb / 10.0;
    const auto fast = fastAttack ? 1.0 : 0.0;
    const auto logRelease = std::log(releaseSeconds / 0.4);
    const auto& slope = kConfig.slopeLaw;
    compressionSlope_ = strength * (slope[0] + inverseRatio * (slope[1] + inverseRatio * slope[2]) +
        threshold * slope[3] + logRelease * slope[4] + limiting * slope[5]);
    const auto& onset = kConfig.onsetLaw;
    detectorOnsetDb_ = onset[0] + inverseRatio * onset[1] + threshold * onset[2] +
        fast * onset[3] + logRelease * onset[4] - kConfig.rmsToPeakDb;
    const auto& bend = kConfig.bendLaw;
    bendDb_ = bend[0] + inverseRatio * (bend[1] + inverseRatio * (bend[2] + inverseRatio * bend[3])) +
        threshold * bend[4] + fast * bend[5] + logRelease * bend[6] +
        fast * logRelease * bend[7] + limiting * bend[8];
    const auto start = DbToGain(detectorOnsetDb_);
    compressionStartPower_ = start * start;
    if (makeupChanged)
    {
        UpdateMakeup(inverseRatio, strength, limiting, threshold);
    }
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
    makeupGain_ = DbToGain(makeupDb);
}

void SslChannelProcessor::UpdateTiming(double releaseSeconds, bool fastAttack) noexcept
{
    attackCoefficient_ = fastAttack ? fastAttackCoefficient_ : normalAttackCoefficient_;
    fastReleaseRetention_ = 1.0 - TimeCoefficient(
        kConfig.fastReleaseFloorSeconds + kConfig.fastReleaseScale * releaseSeconds, sampleRate_);
    const auto slowScale = fastAttack ? kConfig.fastSlowReleaseScale : kConfig.normalSlowReleaseScale;
    const auto slowFloor = fastAttack ? kConfig.fastSlowReleaseFloorSeconds : kConfig.normalSlowReleaseFloorSeconds;
    const auto exponent = fastAttack ? kConfig.fastReleaseExponent : kConfig.normalReleaseExponent;
    slowReleaseRetention_ = 1.0 - TimeCoefficient(
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

} // namespace consolidator::max::dsp::processors

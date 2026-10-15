#include "TransientShaperProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

#include "../../Primitives/Math.h"
#include "../../Primitives/Filters/OnePole.h"
#include "../../Primitives/ChannelView.h"

namespace consolidator::audio::algorithms
{

namespace
{

constexpr double kMaximumGain = 63.095734448;

double AmountGain(double fullGain, double amount) noexcept
{
    // Exact common amounts of fullGain^amount avoid unnecessary power calls.
    if (amount == 0.0)
    {
        return 1.0;
    }
    if (amount == 1.0)
    {
        return fullGain;
    }
    if (amount == -1.0)
    {
        return 1.0 / fullGain;
    }
    if (amount == 0.5)
    {
        return std::sqrt(fullGain);
    }
    if (amount == -0.5)
    {
        return 1.0 / std::sqrt(fullGain);
    }
    return std::pow(fullGain, amount);
}

} // namespace

void TransientShaperProcessor::Prepare(const ProcessingContext& context) noexcept
{
    outputGain_.Prepare(context);
    outputHighpassCoefficient_ = primitives::filters::onepole::FilterCoefficient(10.0, context.sampleRate);
    attackAmount_.Prepare(context);
    sustainAmount_.Prepare(context);
    PrepareSidechainFilter(context.sampleRate);
    PrepareDetectorCoefficients(context.sampleRate);
    Reset();
}

void TransientShaperProcessor::Reset() noexcept
{
    attackAmount_.Reset();
    sustainAmount_.Reset();
    filterStates_ = {};
    outputGain_.Reset();
    outputHighpassStates_ = {};
    fastPower_ = 0.0;
    slowPower_ = initialSlowPower_;
    onsetPower_ = 0.0;
    contextPower_ = 0.0;
    peak_ = 0.0;
    peakDb_ = -240.0;
    attackMask_ = 0.0;
    extendedAttackMask_ = 0.0;
    sustainMask_ = 0.0;
}

void TransientShaperProcessor::SetAmounts(double attack, double sustain) noexcept
{
    attackAmount_.SetTarget(attack);
    sustainAmount_.SetTarget(sustain);
}

void TransientShaperProcessor::SetOutputGainDb(double gainDb) noexcept
{
    if (gainDb != outputGainDb_)
    {
        outputGainDb_ = gainDb;
        outputGain_.SetTarget(primitives::DbToGain(gainDb));
    }
}

double TransientShaperProcessor::FilterSidechainSample(double input, std::size_t channel) noexcept
{
    auto& state = filterStates_[channel];
    const auto output = b0_ * input + state[0];
    state[0] = b1_ * input - a1_ * output + state[1];
    state[1] = b2_ * input - a2_ * output;
    return output;
}

void TransientShaperProcessor::Process(AudioBuffer audio, ChannelMode mode) noexcept
{
    const auto selected = primitives::SelectChannels(audio, mode);
    auto* left = selected.left;
    auto* right = selected.right;
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto magnitude = std::max(
            left != nullptr ? std::abs(FilterSidechainSample(left[index], 0)) : 0.0,
            right != nullptr ? std::abs(FilterSidechainSample(right[index], 1)) : 0.0);
        const auto power = magnitude * magnitude;
        // Detectors and masks advance even when both signed gains are neutral.
        UpdatePowerFollowers(power);
        UpdateHeldPeak(magnitude);
        UpdateMasks(magnitude);
        const auto attack = attackAmount_.NextValue();
        const auto sustain = sustainAmount_.NextValue();
        const auto baseAndSustainGain = CalculateSustainStageGain(sustain);
        const auto attackStageGain = CalculateAttackStageGain(attack, baseAndSustainGain);
        outputGain_.Advance(1);
        const auto outputGain = outputGain_.CurrentValue();
        // Limit Sustain before Attack and the output filter; limit the final output again.
        if (left != nullptr)
        {
            left[index] = ProcessOutputSample(left[index], 0, baseAndSustainGain, attackStageGain, outputGain);
        }
        if (right != nullptr)
        {
            right[index] = ProcessOutputSample(right[index], 1, baseAndSustainGain, attackStageGain, outputGain);
        }
    }
}

void TransientShaperProcessor::PrepareSidechainFilter(double sampleRate) noexcept
{
    const auto& config = kConfig;
    const auto angle = 2.0 * std::numbers::pi * config.sidechainHz / sampleRate;
    const auto cosine = std::cos(angle);
    const auto alpha = std::sin(angle) / (2.0 * 0.71);
    const auto inverseA0 = 1.0 / (1.0 + alpha);
    b0_ = (1.0 + cosine) * 0.5 * inverseA0;
    b1_ = -(1.0 + cosine) * inverseA0;
    b2_ = b0_;
    a1_ = -2.0 * cosine * inverseA0;
    a2_ = (1.0 - alpha) * inverseA0;
}

void TransientShaperProcessor::PrepareDetectorCoefficients(double sampleRate) noexcept
{
    const auto& config = kConfig;
    fastAttackCoefficient_ = primitives::TimeCoefficient(config.fastAttackSeconds, sampleRate);
    fastReleaseCoefficient_ = primitives::TimeCoefficient(config.fastReleaseSeconds, sampleRate);
    slowAttackCoefficient_ = primitives::TimeCoefficient(config.slowAttackSeconds, sampleRate);
    slowReleaseCoefficient_ = primitives::TimeCoefficient(config.slowReleaseSeconds, sampleRate);
    onsetAttackCoefficient_ = primitives::TimeCoefficient(0.0002, sampleRate);
    onsetReleaseCoefficient_ = primitives::TimeCoefficient(0.003, sampleRate);
    contextCoefficient_ = primitives::TimeCoefficient(config.contextSeconds, sampleRate);
    attackOpenCoefficient_ = primitives::TimeCoefficient(0.00001, sampleRate);
    attackReleaseCoefficient_ = primitives::TimeCoefficient(config.attackReleaseSeconds, sampleRate);
    attackAdditionDecay_ = std::exp(-1.0 / (config.attackAdditionReleaseSeconds * sampleRate));
    maskCloseCoefficient_ = primitives::TimeCoefficient(config.maskCloseSeconds, sampleRate);
    sustainOpenCoefficient_ = primitives::TimeCoefficient(config.sustainOpenSeconds, sampleRate);
    peakDecay_ = std::exp(-1.0 / (config.peakReleaseSeconds * sampleRate));
    peakDecayDb_ = 20.0 * std::log10(peakDecay_);
    riseNormalization_ = 1.0 / (std::pow(10.0, config.riseDb / 10.0) - 1.0);
    baseGain_ = primitives::DbToGain(config.baseGainDb);
    initialSlowPower_ = std::pow(10.0, config.initialSlowLevelDb / 10.0);
}

void TransientShaperProcessor::UpdatePowerFollowers(double power) noexcept
{
    fastPower_ += (power > fastPower_ ? fastAttackCoefficient_ : fastReleaseCoefficient_) *
        (power - fastPower_);
    slowPower_ += (power > slowPower_ ? slowAttackCoefficient_ : slowReleaseCoefficient_) *
        (power - slowPower_);
    onsetPower_ += (power > onsetPower_ ? onsetAttackCoefficient_ : onsetReleaseCoefficient_) *
        (power - onsetPower_);
}

void TransientShaperProcessor::UpdateHeldPeak(double magnitude) noexcept
{
    const auto decayedPeak = peak_ * peakDecay_;
    if (magnitude > decayedPeak)
    {
        peak_ = magnitude;
        peakDb_ = 20.0 * std::log10(std::max(peak_, 1.0e-12));
    }
    else
    {
        peak_ = decayedPeak;
        // Exact dB decay avoids recalculating log10 while the held peak falls.
        peakDb_ = std::max(-240.0, peakDb_ + peakDecayDb_);
    }
}

void TransientShaperProcessor::UpdateMasks(double magnitude) noexcept
{
    const auto novelty = std::clamp(
        (onsetPower_ / std::max(contextPower_, 1.0e-24) - 1.0) * riseNormalization_, 0.0, 1.0);
    const auto attackTarget = novelty * magnitude / std::max(peak_, 1.0e-12);
    attackMask_ += (attackTarget > attackMask_ ? attackOpenCoefficient_ : attackReleaseCoefficient_) *
        (attackTarget - attackMask_);
    extendedAttackMask_ = std::max(attackMask_, extendedAttackMask_ * attackAdditionDecay_);
    contextPower_ += contextCoefficient_ * (onsetPower_ - contextPower_);
    const auto activityRatio = fastPower_ / std::max(peak_ * peak_, 1.0e-24);
    const auto activity = activityRatio >= 1.0
        ? 1.0 : std::sqrt(std::sqrt(std::sqrt(std::sqrt(activityRatio))));
    const auto sustainTarget = (1.0 - attackMask_) * activity;
    sustainMask_ += (sustainTarget > sustainMask_ ? sustainOpenCoefficient_ : maskCloseCoefficient_) *
        (sustainTarget - sustainMask_);
}

double TransientShaperProcessor::CalculateSustainStageGain(double sustain) const noexcept
{
    auto sustainGain = 0.0;
    if (sustain != 0.0)
    {
        const auto strength = std::clamp(
            1.0 + peakDb_ / kConfig.strengthRangeDb, 0.0, 1.0);
        const auto slowPower = std::max(slowPower_, 1.0e-24);
        const auto fastPower = std::max(fastPower_, 1.0e-24);
        const auto excess = slowPower > fastPower && strength > 0.0
            ? std::pow(slowPower / fastPower, kConfig.tailExponent * strength) - 1.0 : 0.0;
        sustainGain = kConfig.steadyReturn + kConfig.tailBoost * excess;
    }
    const auto fullSustainGain = std::min(kMaximumGain,
        baseGain_ + sustainMask_ * sustainGain);
    const auto baseAndSustainGain = AmountGain(fullSustainGain, sustain);
    return baseAndSustainGain;
}

double TransientShaperProcessor::CalculateAttackStageGain(
    double attack, double baseAndSustainGain) const noexcept
{
    const auto attackAdditionMask = attackMask_ + kConfig.attackAdditionMix *
        (extendedAttackMask_ - attackMask_);
    const auto fullAttackGain = 1.0 + kConfig.attackBoost * attackAdditionMask;
    const auto attackGain = AmountGain(fullAttackGain, attack);
    const auto requestedGain = baseAndSustainGain * attackGain;
    const auto combinedGain = std::clamp(requestedGain, 1.0 / kMaximumGain, kMaximumGain);
    const auto attackStageGain = combinedGain == requestedGain
        ? attackGain : combinedGain / baseAndSustainGain;
    return attackStageGain;
}

double TransientShaperProcessor::ProcessOutputSample(
    double input, std::size_t channel, double sustainGain, double attackGain, double outputGain) noexcept
{
    // Clamp before the output HP: moving this clamp excites a different low-frequency tail.
    const auto limitedSustain = std::clamp(input * sustainGain, -1.0, 1.0);
    const auto output = outputGain * primitives::filters::onepole::HighPass(
        limitedSustain * attackGain, outputHighpassCoefficient_, outputHighpassStates_[channel]);
    return std::clamp(output, -1.0, 1.0);
}

} // namespace consolidator::audio::algorithms

#include "TransientShaperProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

#include "DspMath.h"
#include "DspOnePole.h"
#include "DspProcessingPrimitives.h"

namespace consolidator::max::dsp
{

namespace
{

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

void TransientShaperProcessor::SetTuningBeforePrepare(const TransientShaperTuning& tuning) noexcept
{
    tuning_ = tuning;
}

void TransientShaperProcessor::Prepare(const DspProcessingContext& context) noexcept
{
    const auto& tuning = tuning_;
    outputGain_.Prepare(context);
    outputHighpassCoefficient_ = onepole::FilterCoefficient(10.0, context.sampleRate);
    attackAmount_.Prepare(context);
    sustainAmount_.Prepare(context);
    const auto angle = 2.0 * std::numbers::pi * tuning.sidechainHz / context.sampleRate;
    const auto cosine = std::cos(angle);
    const auto alpha = std::sin(angle) / (2.0 * 0.71);
    const auto inverseA0 = 1.0 / (1.0 + alpha);
    b0_ = (1.0 + cosine) * 0.5 * inverseA0;
    b1_ = -(1.0 + cosine) * inverseA0;
    b2_ = b0_;
    a1_ = -2.0 * cosine * inverseA0;
    a2_ = (1.0 - alpha) * inverseA0;
    fastAttackCoefficient_ = TimeCoefficient(tuning.fastAttackSeconds, context.sampleRate);
    fastReleaseCoefficient_ = TimeCoefficient(tuning.fastReleaseSeconds, context.sampleRate);
    slowAttackCoefficient_ = TimeCoefficient(tuning.slowAttackSeconds, context.sampleRate);
    slowReleaseCoefficient_ = TimeCoefficient(tuning.slowReleaseSeconds, context.sampleRate);
    onsetAttackCoefficient_ = TimeCoefficient(0.0002, context.sampleRate);
    onsetReleaseCoefficient_ = TimeCoefficient(0.003, context.sampleRate);
    contextCoefficient_ = TimeCoefficient(tuning.contextSeconds, context.sampleRate);
    attackOpenCoefficient_ = TimeCoefficient(0.00001, context.sampleRate);
    attackReleaseCoefficient_ = TimeCoefficient(tuning.attackReleaseSeconds, context.sampleRate);
    attackAdditionDecay_ = std::exp(-1.0 / (tuning.attackAdditionReleaseSeconds * context.sampleRate));
    maskCloseCoefficient_ = TimeCoefficient(tuning.maskCloseSeconds, context.sampleRate);
    sustainOpenCoefficient_ = TimeCoefficient(tuning.sustainOpenSeconds, context.sampleRate);
    peakDecay_ = std::exp(-1.0 / (tuning.peakReleaseSeconds * context.sampleRate));
    peakDecayDb_ = 20.0 * std::log10(peakDecay_);
    riseNormalization_ = 1.0 / (std::pow(10.0, tuning.riseDb / 10.0) - 1.0);
    baseGain_ = DbToGain(tuning.baseGainDb);
    initialSlowPower_ = std::pow(10.0, tuning.initialSlowLevelDb / 10.0);
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

void TransientShaperProcessor::SetParameters(const DspSnapshot& snapshot) noexcept
{
    SetAmounts(snapshot.compressorAttack, snapshot.compressorSustain);
    SetOutputGainDb(snapshot.compressorOutputDb);
}

void TransientShaperProcessor::SetOutputGainDb(double gainDb) noexcept
{
    outputGain_.SetTarget(DbToGain(gainDb));
}

void TransientShaperProcessor::Process(DspProcessingFrame& frame) noexcept
{
    Process(frame.main, frame.plan.channelMode);
}

double TransientShaperProcessor::Filter(double input, std::size_t channel) noexcept
{
    auto& state = filterStates_[channel];
    const auto output = b0_ * input + state[0];
    state[0] = b1_ * input - a1_ * output + state[1];
    state[1] = b2_ * input - a2_ * output;
    return output;
}

void TransientShaperProcessor::Process(DspAudioBuffer audio, DspChannelMode mode) noexcept
{
    const auto selected = SelectChannels(audio, mode);
    auto* left = selected.left;
    auto* right = selected.right;
    constexpr auto maximumGain = 63.095734448;
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto magnitude = std::max(
            left != nullptr ? std::abs(Filter(left[index], 0)) : 0.0,
            right != nullptr ? std::abs(Filter(right[index], 1)) : 0.0);
        const auto power = magnitude * magnitude;
        fastPower_ += (power > fastPower_ ? fastAttackCoefficient_ : fastReleaseCoefficient_) *
            (power - fastPower_);
        slowPower_ += (power > slowPower_ ? slowAttackCoefficient_ : slowReleaseCoefficient_) *
            (power - slowPower_);
        onsetPower_ += (power > onsetPower_ ? onsetAttackCoefficient_ : onsetReleaseCoefficient_) *
            (power - onsetPower_);
        const auto decayedPeak = peak_ * peakDecay_;
        if (magnitude > decayedPeak)
        {
            peak_ = magnitude;
            peakDb_ = 20.0 * std::log10(std::max(peak_, 1.0e-12));
        }
        else
        {
            peak_ = decayedPeak;
            peakDb_ = std::max(-240.0, peakDb_ + peakDecayDb_);
        }
        const auto novelty = std::clamp(
            (onsetPower_ / std::max(contextPower_, 1.0e-24) - 1.0) * riseNormalization_, 0.0, 1.0);
        const auto attackTarget = novelty * magnitude / std::max(peak_, 1.0e-12);
        attackMask_ += (attackTarget > attackMask_ ? attackOpenCoefficient_ : attackReleaseCoefficient_) *
            (attackTarget - attackMask_);
        extendedAttackMask_ = std::max(attackMask_, extendedAttackMask_ * attackAdditionDecay_);
        contextPower_ += contextCoefficient_ * (onsetPower_ - contextPower_);
        const auto activityRatio = fastPower_ / std::max(peak_ * peak_, 1.0e-24);
        const auto activity = activityRatio >= 1.0
            ? 1.0 : std::pow(activityRatio, tuning_.activityExponent);
        const auto sustainTarget = (1.0 - attackMask_) * activity;
        sustainMask_ += (sustainTarget > sustainMask_ ? sustainOpenCoefficient_ : maskCloseCoefficient_) *
            (sustainTarget - sustainMask_);
        const auto attack = attackAmount_.NextValue();
        const auto sustain = sustainAmount_.NextValue();
        auto sustainGain = 0.0;
        if (sustain != 0.0)
        {
            const auto strength = std::clamp(
                1.0 + peakDb_ / tuning_.strengthRangeDb, 0.0, 1.0);
            const auto slowPower = std::max(slowPower_, 1.0e-24);
            const auto fastPower = std::max(fastPower_, 1.0e-24);
            const auto excess = slowPower > fastPower && strength > 0.0
                ? std::pow(slowPower / fastPower, tuning_.tailExponent * strength) - 1.0 : 0.0;
            sustainGain = tuning_.steadyReturn + tuning_.tailBoost * excess;
        }
        const auto fullSustainGain = std::min(maximumGain,
            baseGain_ + sustainMask_ * sustainGain);
        const auto baseAndSustainGain = AmountGain(fullSustainGain, sustain);
        const auto attackAdditionMask = attackMask_ + tuning_.attackAdditionMix *
            (extendedAttackMask_ - attackMask_);
        const auto fullAttackGain = 1.0 + tuning_.attackBoost * attackAdditionMask;
        const auto combinedGain = std::clamp(
            baseAndSustainGain * AmountGain(fullAttackGain, attack), 1.0 / maximumGain, maximumGain);
        const auto attackStageGain = combinedGain / baseAndSustainGain;
        outputGain_.Advance(1);
        const auto outputGain = outputGain_.CurrentValue();
        // Limit Sustain before Attack and the output filter; limit the final output again.
        if (left != nullptr)
        {
            const auto sustainLeft = std::clamp(left[index] * baseAndSustainGain, -1.0, 1.0);
            const auto outputLeft = outputGain * onepole::HighPass(
                sustainLeft * attackStageGain, outputHighpassCoefficient_, outputHighpassStates_[0]);
            left[index] = std::clamp(outputLeft, -1.0, 1.0);
        }
        if (right != nullptr)
        {
            const auto sustainRight = std::clamp(right[index] * baseAndSustainGain, -1.0, 1.0);
            const auto outputRight = outputGain * onepole::HighPass(
                sustainRight * attackStageGain, outputHighpassCoefficient_, outputHighpassStates_[1]);
            right[index] = std::clamp(outputRight, -1.0, 1.0);
        }
    }
}

} // namespace consolidator::max::dsp

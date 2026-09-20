#include "TransientShaperProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "DspMath.h"
#include "DspProcessingPrimitives.h"

namespace
{
[[nodiscard]] double FollowEnvelope(
    double input,
    double envelope,
    double coefficient) noexcept
{
    return envelope + coefficient * (input - envelope);
}

}

namespace consolidator::max::dsp
{

void TransientShaperProcessor::SetTuningBeforePrepare(
    const TransientShaperTuning& tuning) noexcept
{
    tuning_ = tuning;
}

void TransientShaperProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    attack_.Prepare(context);
    sustain_.Prepare(context);
    activeMix_.Prepare(context);
    autoGain_.SetTuningBeforePrepare(tuning_.autoGain);
    autoGain_.Prepare(context);
    sustainCoefficient_ = TimeCoefficient(
        tuning_.sustainEnvelopeSeconds, context.sampleRate);
    fastAttackCoefficient_ = TimeCoefficient(
        tuning_.fastEnvelopeSeconds, context.sampleRate);
    slowAttackCoefficient_ = TimeCoefficient(
        tuning_.slowEnvelopeSeconds, context.sampleRate);
    detectorCoefficient_ = TimeCoefficient(
        tuning_.detectorSeconds, context.sampleRate);
    // Symmetric followers have the same DC gain; different time constants
    // then measure envelope motion instead of different peak biases.
    gainAttackCoefficient_ = TimeCoefficient(
        tuning_.gainAttackSeconds, context.sampleRate);
    gainReleaseCoefficient_ = TimeCoefficient(
        tuning_.gainReleaseSeconds, context.sampleRate);
}

void TransientShaperProcessor::Reset() noexcept
{
    attack_.Reset();
    sustain_.Reset();
    activeMix_.Reset();
    fastEnvelope_ = 0.0;
    slowEnvelope_ = 0.0;
    sustainEnvelope_ = 0.0;
    autoGain_.Reset();
    gainDb_ = 0.0;
    detectorEnergy_ = 0.0;
    detectorGateOpen_ = false;
}

void TransientShaperProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    attack_.SetTarget(snapshot.compressorAttack);
    sustain_.SetTarget(snapshot.compressorSustain);
    activeMix_.SetTarget(snapshot.compressorAttack > 0.0F || snapshot.compressorSustain > 0.0F ? 1.0 : 0.0);
}

void TransientShaperProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    const auto processLeft = frame.plan.channelMode != DspChannelMode::Right;
    const auto processRight = frame.plan.channelMode != DspChannelMode::Left;
    const auto selected = SelectChannels(frame.main, frame.plan.channelMode);
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        const auto left = !processLeft || frame.detectorSignal.left == nullptr
            ? 0.0
            : std::abs(frame.detectorSignal.left[index]);
        const auto right = !processRight || frame.detectorSignal.right == nullptr
            ? 0.0
            : std::abs(frame.detectorSignal.right[index]);
        const auto magnitude = std::max(left, right);
        detectorEnergy_ += detectorCoefficient_ * (magnitude * magnitude - detectorEnergy_);
        const auto detectorLevel = std::sqrt(std::max(0.0, detectorEnergy_));
        if (detectorLevel < tuning_.minimumDetectorLevel)
        {
            detectorGateOpen_ = false;
        }
        else if (detectorLevel > 2.0 * tuning_.minimumDetectorLevel)
        {
            detectorGateOpen_ = true;
        }

        fastEnvelope_ = FollowEnvelope(
            detectorLevel,
            fastEnvelope_,
            fastAttackCoefficient_);
        slowEnvelope_ = FollowEnvelope(
            detectorLevel,
            slowEnvelope_,
            slowAttackCoefficient_);
        sustainEnvelope_ = FollowEnvelope(detectorLevel, sustainEnvelope_, sustainCoefficient_);

        const auto attack = std::clamp(attack_.NextValue(), 0.0, 1.0);
        const auto sustain = std::clamp(sustain_.NextValue(), 0.0, 1.0);
        auto targetGainDb = 0.0;
        if (detectorGateOpen_ &&
            fastEnvelope_ >= tuning_.minimumDetectorLevel &&
            slowEnvelope_ >= tuning_.minimumDetectorLevel)
        {
            const auto envelopeDifferenceDb = 20.0 * std::log10(
                fastEnvelope_ / slowEnvelope_);
            const auto attackDb = std::clamp(
                tuning_.attackScale * envelopeDifferenceDb,
                0.0,
                tuning_.maximumAttackDb);
            const auto sustainDb = std::clamp(
                tuning_.sustainScale * 20.0 * std::log10(
                    std::max(sustainEnvelope_, tuning_.minimumDetectorLevel) / fastEnvelope_),
                0.0,
                tuning_.maximumSustainDb);
            targetGainDb =
                attack * attackDb +
                sustain * sustainDb;
        }

        const auto gainCoefficient = targetGainDb > gainDb_
            ? gainAttackCoefficient_
            : gainReleaseCoefficient_;
        gainDb_ += gainCoefficient * (targetGainDb - gainDb_);
        const auto activity = activeMix_.NextValue();
        const auto gain = DbToGain(activity * gainDb_);
        const auto inputEnergy = selected.MeanSquare(index);
        const auto compensatedGain = gain * autoGain_.Process(inputEnergy, inputEnergy * gain * gain, activity);

        selected.ApplyGain(index, compensatedGain);
    }
}

} // namespace consolidator::max::dsp

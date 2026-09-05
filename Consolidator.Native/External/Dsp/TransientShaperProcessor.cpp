#include "DspProcessors.h"

#include <algorithm>
#include <cmath>

namespace
{

constexpr double kFastAttackSeconds = 0.002;
constexpr double kSlowAttackSeconds = 0.025;
constexpr double kGainAttackSeconds = 0.001;
constexpr double kGainReleaseSeconds = 0.050;
constexpr double kMaximumAttackDb = 7.5;
constexpr double kMaximumSustainDb = 9.0;
constexpr double kMinimumDetectorLevel = 3.1622776601683795e-5;

[[nodiscard]] double EnvelopeCoefficient(
    double sampleRate,
    double timeSeconds) noexcept
{
    return 1.0 - std::exp(-1.0 / (sampleRate * timeSeconds));
}

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

void TransientShaperProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    attack_.Prepare(context);
    sustain_.Prepare(context);
    activeMix_.Prepare(context);
    autoGain_.Prepare(context);
    sustainCoefficient_ = EnvelopeCoefficient(context.sampleRate, 0.120);
    fastAttackCoefficient_ = EnvelopeCoefficient(
        context.sampleRate,
        kFastAttackSeconds);
    slowAttackCoefficient_ = EnvelopeCoefficient(
        context.sampleRate,
        kSlowAttackSeconds);
    detectorCoefficient_ = EnvelopeCoefficient(context.sampleRate, 0.005);
    // Symmetric followers have the same DC gain; different time constants
    // then measure envelope motion instead of different peak biases.
    gainAttackCoefficient_ = EnvelopeCoefficient(
        context.sampleRate,
        kGainAttackSeconds);
    gainReleaseCoefficient_ = EnvelopeCoefficient(
        context.sampleRate,
        kGainReleaseSeconds);
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
        if (detectorLevel < kMinimumDetectorLevel)
        {
            detectorGateOpen_ = false;
        }
        else if (detectorLevel > 2.0 * kMinimumDetectorLevel)
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
            fastEnvelope_ >= kMinimumDetectorLevel &&
            slowEnvelope_ >= kMinimumDetectorLevel)
        {
            const auto envelopeDifferenceDb = 20.0 * std::log10(
                fastEnvelope_ / slowEnvelope_);
            const auto attackDb = std::clamp(
                1.25 * envelopeDifferenceDb,
                0.0,
                kMaximumAttackDb);
            const auto sustainDb = std::clamp(
                1.5 * 20.0 * std::log10(
                    std::max(sustainEnvelope_, kMinimumDetectorLevel) / fastEnvelope_),
                0.0,
                kMaximumSustainDb);
            targetGainDb =
                attack * attackDb +
                sustain * sustainDb;
        }

        const auto gainCoefficient = targetGainDb > gainDb_
            ? gainAttackCoefficient_
            : gainReleaseCoefficient_;
        gainDb_ += gainCoefficient * (targetGainDb - gainDb_);
        const auto activity = activeMix_.NextValue();
        const auto gain = std::pow(10.0, activity * gainDb_ / 20.0);
        auto inputEnergy = 0.0;
        auto channelCount = 0.0;
        if (processLeft && frame.main.left != nullptr)
        {
            inputEnergy += frame.main.left[index] * frame.main.left[index];
            channelCount += 1.0;
        }
        if (processRight && frame.main.right != nullptr)
        {
            inputEnergy += frame.main.right[index] * frame.main.right[index];
            channelCount += 1.0;
        }
        inputEnergy /= std::max(channelCount, 1.0);
        const auto compensatedGain = gain * autoGain_.Process(inputEnergy, inputEnergy * gain * gain, activity);

        if (processLeft && frame.main.left != nullptr)
        {
            frame.main.left[index] *= compensatedGain;
        }
        if (processRight && frame.main.right != nullptr)
        {
            frame.main.right[index] *= compensatedGain;
        }
    }
}

} // namespace consolidator::max::dsp

#include "DspProcessors.h"

#include <algorithm>
#include <cmath>

namespace
{

constexpr double kLowCrossoverHz = 250.0;
constexpr double kHighCrossoverHz = 2500.0;
constexpr double kEnvelopeReleaseSeconds = 0.500;
constexpr double kMinimumEnvelope = 1.0e-12;
constexpr double kPi = 3.14159265358979323846;

[[nodiscard]] double CrossoverCoefficient(
    double frequencyHz,
    double sampleRate) noexcept
{
    return 1.0 - std::exp(-2.0 * kPi * frequencyHz / sampleRate);
}

[[nodiscard]] double LowPass(
    double input,
    double coefficient,
    double& state) noexcept
{
    state += coefficient * (input - state);
    return state;
}

[[nodiscard]] double ProcessBand(
    double sample,
    double envelope,
    double curve,
    double& previousSample) noexcept
{
    if (envelope <= kMinimumEnvelope)
    {
        previousSample = sample;
        return sample;
    }
    const auto strength = 0.75 + 1.25 * curve;
    const auto current = sample / envelope;
    const auto previous = previousSample / envelope;
    previousSample = sample;
    // First-order ADAA of the nonlinear residual. Rationalizing the
    // antiderivative difference avoids cancellation for adjacent samples.
    const auto currentRoot = std::sqrt(1.0 + strength * strength * current * current);
    const auto previousRoot = std::sqrt(1.0 + strength * strength * previous * previous);
    const auto average = 0.5 * (current + previous);
    const auto shapedAverage = std::sqrt(1.0 + strength * strength) *
        (current + previous) / (currentRoot + previousRoot);
    return sample + envelope * (shapedAverage - average);
}

}

namespace consolidator::max::dsp
{

void SaturationProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    drive_.Prepare(context);
    curve_.Prepare(context);
    splitMix_.Prepare(context);
    autoGain_.Prepare(context);
    envelopeReleaseCoefficient_ = std::exp(
        -1.0 / (context.sampleRate * kEnvelopeReleaseSeconds));
    lowCrossoverCoefficient_ = CrossoverCoefficient(
        kLowCrossoverHz,
        context.sampleRate);
    highCrossoverCoefficient_ = CrossoverCoefficient(
        kHighCrossoverHz,
        context.sampleRate);
}

void SaturationProcessor::Reset() noexcept
{
    drive_.Reset();
    curve_.Reset();
    autoGain_.Reset();
    const auto splitTarget = splitMix_.TargetValue();
    splitMix_.SetTarget(0.0);
    splitMix_.Reset();
    splitMix_.SetTarget(splitTarget);
    splitState_ = {};
    detectorEnvelope_ = 0.0;
    previousSamples_ = {};
    envelopeWeight_ = 0.0;
}

void SaturationProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    drive_.SetTarget(snapshot.saturatorDrive);
    curve_.SetTarget(snapshot.saturatorCurve);
    splitMix_.SetTarget(snapshot.saturatorSplit != 0 ? 1.0 : 0.0);
}

void SaturationProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    const auto processLeft = frame.plan.channelMode != DspChannelMode::Right;
    const auto processRight = frame.plan.channelMode != DspChannelMode::Left;
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        auto detectorMagnitude = 0.0;
        if (processLeft && frame.detectorSignal.left != nullptr)
        {
            detectorMagnitude = std::abs(frame.detectorSignal.left[index]);
        }
        if (processRight && frame.detectorSignal.right != nullptr)
        {
            detectorMagnitude = std::max(
                detectorMagnitude,
                std::abs(frame.detectorSignal.right[index]));
        }

        const auto energy = detectorMagnitude * detectorMagnitude;
        if (energy > 1.0e-12)
        {
            detectorEnvelope_ =
                envelopeReleaseCoefficient_ * detectorEnvelope_ +
                (1.0 - envelopeReleaseCoefficient_) * energy;
            envelopeWeight_ = envelopeReleaseCoefficient_ * envelopeWeight_ +
                (1.0 - envelopeReleaseCoefficient_);
        }
        // Normalize the startup window's weight so its first quiet sample
        // does not establish an artificially low reference for half a second.
        const auto normalization = std::sqrt(
            2.0 * detectorEnvelope_ / std::max(envelopeWeight_, 1.0e-12));

        const auto drive = std::clamp(drive_.NextValue(), 0.0, 1.0);
        const auto curve = std::clamp(curve_.NextValue(), 0.0, 1.0);
        const auto splitMix = splitMix_.NextValue();
        auto inputEnergy = 0.0;
        auto outputEnergy = 0.0;
        auto channelCount = 0.0;
        if (processLeft && frame.main.left != nullptr)
        {
            auto& sample = frame.main.left[index];
            const auto dry = sample;
            inputEnergy += dry * dry;
            channelCount += 1.0;
            const auto wide = ProcessBand(dry, normalization, curve, previousSamples_[0]);
            const auto low = LowPass(
                dry,
                lowCrossoverCoefficient_,
                splitState_.lowLeft);
            const auto highInput = dry - low;
            const auto mid = LowPass(
                highInput,
                highCrossoverCoefficient_,
                splitState_.midLeft);
            const auto high = highInput - mid;
            const auto split =
                ProcessBand(low, normalization, curve, previousSamples_[1]) +
                ProcessBand(mid, normalization, curve, previousSamples_[2]) +
                ProcessBand(high, normalization, curve, previousSamples_[3]);
            const auto wet = wide + splitMix * (split - wide);
            sample = dry + drive * (wet - dry);
            outputEnergy += sample * sample;
        }
        if (processRight && frame.main.right != nullptr)
        {
            auto& sample = frame.main.right[index];
            const auto dry = sample;
            inputEnergy += dry * dry;
            channelCount += 1.0;
            const auto wide = ProcessBand(dry, normalization, curve, previousSamples_[4]);
            const auto low = LowPass(
                dry,
                lowCrossoverCoefficient_,
                splitState_.lowRight);
            const auto highInput = dry - low;
            const auto mid = LowPass(
                highInput,
                highCrossoverCoefficient_,
                splitState_.midRight);
            const auto high = highInput - mid;
            const auto split =
                ProcessBand(low, normalization, curve, previousSamples_[5]) +
                ProcessBand(mid, normalization, curve, previousSamples_[6]) +
                ProcessBand(high, normalization, curve, previousSamples_[7]);
            const auto wet = wide + splitMix * (split - wide);
            sample = dry + drive * (wet - dry);
            outputEnergy += sample * sample;
        }
        const auto divisor = std::max(channelCount, 1.0);
        const auto compensation = autoGain_.Process(inputEnergy / divisor, outputEnergy / divisor, drive);
        if (processLeft && frame.main.left != nullptr)
        {
            frame.main.left[index] *= compensation;
        }
        if (processRight && frame.main.right != nullptr)
        {
            frame.main.right[index] *= compensation;
        }
    }
}

} // namespace consolidator::max::dsp

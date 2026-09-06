#include "SaturationProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "DspProcessingPrimitives.h"

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
    double strengthSquared,
    double shapeGain,
    bool enabled,
    double& previousSample) noexcept
{
    if (!enabled || envelope <= kMinimumEnvelope)
    {
        previousSample = sample;
        return sample;
    }
    const auto current = sample / envelope;
    const auto previous = previousSample / envelope;
    previousSample = sample;
    // First-order ADAA of the nonlinear residual. Rationalizing the
    // antiderivative difference avoids cancellation for adjacent samples.
    const auto currentRoot = std::sqrt(1.0 + strengthSquared * current * current);
    const auto previousRoot = std::sqrt(1.0 + strengthSquared * previous * previous);
    const auto average = 0.5 * (current + previous);
    const auto shapedAverage = shapeGain *
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
    channels_ = {};
    detectorEnvelope_ = 0.0;
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
    const auto selected = SelectChannels(frame.main, frame.plan.channelMode);
    const std::array<double*, 2> audio{ selected.left, selected.right };
    const auto divisor = static_cast<double>(std::max(selected.channelCount, std::size_t{ 1 }));
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
        const auto strength = 0.75 + 1.25 * curve;
        const auto strengthSquared = strength * strength;
        const auto shapeGain = std::sqrt(1.0 + strengthSquared);
        const auto wideEnabled = drive > 0.0 && splitMix < 1.0;
        const auto splitEnabled = drive > 0.0 && splitMix > 0.0;
        auto inputEnergy = 0.0;
        auto outputEnergy = 0.0;
        for (std::size_t channel = 0; channel < audio.size(); ++channel)
        {
            if (audio[channel] == nullptr)
            {
                continue;
            }
            auto& sample = audio[channel][index];
            const auto dry = sample;
            auto& state = channels_[channel];
            inputEnergy += dry * dry;
            const auto wide = ProcessBand(dry, normalization, strengthSquared, shapeGain,
                wideEnabled, state.previousSamples[0]);
            // Crossovers and physical-sample history stay warm even when a
            // nonlinear branch is inaudible. Only its square roots are skipped.
            const auto low = LowPass(dry, lowCrossoverCoefficient_, state.low);
            const auto highInput = dry - low;
            const auto mid = LowPass(highInput, highCrossoverCoefficient_, state.mid);
            const auto high = highInput - mid;
            const auto split =
                ProcessBand(low, normalization, strengthSquared, shapeGain, splitEnabled, state.previousSamples[1]) +
                ProcessBand(mid, normalization, strengthSquared, shapeGain, splitEnabled, state.previousSamples[2]) +
                ProcessBand(high, normalization, strengthSquared, shapeGain, splitEnabled, state.previousSamples[3]);
            const auto wet = wide + splitMix * (split - wide);
            sample = dry + drive * (wet - dry);
            outputEnergy += sample * sample;
        }
        const auto compensation = autoGain_.Process(inputEnergy / divisor, outputEnergy / divisor, drive);
        selected.ApplyGain(index, compensation);
    }
}

} // namespace consolidator::max::dsp

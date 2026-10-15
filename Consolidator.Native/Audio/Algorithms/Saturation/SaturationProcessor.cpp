#include "SaturationProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "../../Primitives/ChannelView.h"

namespace
{

constexpr double kLowCrossoverHz = 250.0;
constexpr double kHighCrossoverHz = 2500.0;
constexpr double kEnvelopeReleaseSeconds = 0.500;
constexpr double kMinimumEnvelope = 1.0e-12;

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

namespace consolidator::audio::algorithms
{

void SaturationProcessor::Prepare(
    const ProcessingContext& context) noexcept
{
    drive_.Prepare(context);
    curve_.Prepare(context);
    splitMix_.Prepare(context);
    envelopeReleaseCoefficient_ = std::exp(
        -1.0 / (context.sampleRate * kEnvelopeReleaseSeconds));
    for (auto& channel : channels_)
    {
        channel.low.Prepare(kLowCrossoverHz, context.sampleRate);
        channel.mid.Prepare(kHighCrossoverHz, context.sampleRate);
    }
}

void SaturationProcessor::Reset() noexcept
{
    drive_.Reset();
    curve_.Reset();
    const auto splitTarget = splitMix_.TargetValue();
    splitMix_.SetTarget(0.0);
    splitMix_.Reset();
    splitMix_.SetTarget(splitTarget);
    for (auto& channel : channels_)
    {
        channel.low.Reset();
        channel.mid.Reset();
        channel.previousSamples = {};
    }
    detectorEnvelope_ = 0.0;
    envelopeWeight_ = 0.0;
}

void SaturationProcessor::SetControls(
    double drive, double curve, bool split) noexcept
{
    drive_.SetTarget(drive);
    curve_.SetTarget(curve);
    splitMix_.SetTarget(split ? 1.0 : 0.0);
}

void SaturationProcessor::Process(
    AudioBuffer buffer, DetectorBuffer detector, ChannelMode mode) noexcept
{
    const auto processLeft = mode != ChannelMode::Right;
    const auto processRight = mode != ChannelMode::Left;
    const auto selected = primitives::SelectChannels(buffer, mode);
    const std::array<double*, 2> audio{ selected.left, selected.right };
    for (std::size_t index = 0; index < buffer.frameCount; ++index)
    {
        auto detectorMagnitude = 0.0;
        if (processLeft && detector.left != nullptr)
        {
            detectorMagnitude = std::abs(detector.left[index]);
        }
        if (processRight && detector.right != nullptr)
        {
            detectorMagnitude = std::max(
                detectorMagnitude,
                std::abs(detector.right[index]));
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
        for (std::size_t channel = 0; channel < audio.size(); ++channel)
        {
            if (audio[channel] == nullptr)
            {
                continue;
            }
            auto& sample = audio[channel][index];
            const auto dry = sample;
            auto& state = channels_[channel];
            const auto wide = ProcessBand(dry, normalization, strengthSquared, shapeGain,
                wideEnabled, state.previousSamples[0]);
            // Crossovers and physical-sample history stay warm even when a
            // nonlinear branch is inaudible. Only its square roots are skipped.
            const auto low = state.low.Process(dry);
            const auto highInput = dry - low;
            const auto mid = state.mid.Process(highInput);
            const auto high = highInput - mid;
            const auto split =
                ProcessBand(low, normalization, strengthSquared, shapeGain, splitEnabled, state.previousSamples[1]) +
                ProcessBand(mid, normalization, strengthSquared, shapeGain, splitEnabled, state.previousSamples[2]) +
                ProcessBand(high, normalization, strengthSquared, shapeGain, splitEnabled, state.previousSamples[3]);
            const auto wet = wide + splitMix * (split - wide);
            sample = dry + drive * (wet - dry);
        }
    }
}

} // namespace consolidator::audio::algorithms

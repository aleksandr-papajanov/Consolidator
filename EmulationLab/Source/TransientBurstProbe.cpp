#include "Probe.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <utility>

namespace emulation_lab
{

namespace
{

constexpr double kMinimumAmplitude = 1.0e-15;

std::size_t ToSamples(double seconds, double sampleRate)
{
    return static_cast<std::size_t>(std::ceil(std::max(seconds, 0.0) * sampleRate));
}

std::vector<double> RmsEnvelope(std::span<const double> samples, std::size_t window)
{
    window = std::max<std::size_t>(window, 1);
    std::vector<double> envelope(samples.size());
    auto energy = 0.0;
    for (std::size_t index = 0; index < samples.size(); ++index)
    {
        energy += samples[index] * samples[index];
        if (index >= window)
        {
            energy -= samples[index - window] * samples[index - window];
        }
        const auto count = std::min(index + 1, window);
        envelope[index] = std::sqrt(std::max(energy, 0.0) / static_cast<double>(count));
    }
    return envelope;
}

double Average(std::span<const double> values)
{
    auto sum = 0.0;
    for (const auto value : values)
    {
        sum += value;
    }
    return values.empty() ? 0.0 : sum / static_cast<double>(values.size());
}

double ToDecibels(double value) noexcept
{
    return 20.0 * std::log10(std::max(value, kMinimumAmplitude));
}

} // namespace

TransientBurstProbe::TransientBurstProbe(TransientBurstConfiguration configuration)
    : configuration_(std::move(configuration))
{
}

ProbeResult TransientBurstProbe::Run(
    const ProbeContext& context,
    const ProcessorCallbacks& processor,
    double frequency,
    double levelDb) const
{
    if (!processor.process || context.sampleRate <= 0.0)
    {
        throw std::invalid_argument("Transient probe needs a processor and a valid sample rate.");
    }
    if (processor.reset)
    {
        processor.reset();
    }
    const auto silence = ToSamples(configuration_.silenceSeconds, context.sampleRate);
    const auto sustain = ToSamples(configuration_.sustainSeconds, context.sampleRate);
    const auto tail = ToSamples(configuration_.tailSeconds, context.sampleRate);
    const auto latency = processor.latencySamples ? processor.latencySamples() : 0;
    const auto amplitude = std::pow(10.0, levelDb / 20.0);
    std::vector<double> input(silence + sustain + tail + latency);
    for (std::size_t index = silence; index < silence + sustain; ++index)
    {
        input[index] = amplitude * std::sin(2.0 * std::numbers::pi * frequency *
            static_cast<double>(index - silence) / context.sampleRate);
    }
    const auto output = processor.process(input);
    if (output.size() != input.size())
    {
        throw std::runtime_error("Processor returned a short transient buffer.");
    }
    const auto window = std::max<std::size_t>(
        ToSamples(configuration_.envelopeWindowSeconds, context.sampleRate), 1);
    const auto envelope = RmsEnvelope(output, window);
    const auto onset = std::min(silence + latency, envelope.size());
    const auto release = std::min(silence + sustain + latency, envelope.size());
    const auto steadyStart = onset + sustain * 3 / 4;
    const auto steadyEnd = std::min(release, envelope.size());
    const auto steady = Average(std::span<const double>(
        envelope.data() + std::min(steadyStart, envelope.size()),
        steadyEnd > steadyStart ? steadyEnd - steadyStart : 0));
    const auto attackTarget = steady * 0.6321205588;
    const auto releaseTarget = steady * 0.3678794412;
    auto attackCrossing = onset;
    for (auto index = onset; index < steadyEnd; ++index)
    {
        if (envelope[index] >= attackTarget)
        {
            attackCrossing = index;
            break;
        }
    }
    auto releaseCrossing = release;
    for (auto index = release; index < envelope.size(); ++index)
    {
        if (envelope[index] <= releaseTarget)
        {
            releaseCrossing = index;
            break;
        }
    }
    const auto attackMs = 1000.0 * static_cast<double>(attackCrossing - onset) / context.sampleRate;
    const auto releaseMs = 1000.0 * static_cast<double>(releaseCrossing - release) / context.sampleRate;
    auto peak = 0.0;
    for (auto index = onset; index < steadyEnd; ++index)
    {
        peak = std::max(peak, std::abs(output[index]));
    }
    std::vector<ProbeFeature> features{
        { "attack.ms", "envelope", "ms", attackMs, 5.0 },
        { "release.ms", "envelope", "ms", releaseMs, 20.0 },
        { "sustain.dbfs", "rms", "dBFS", ToDecibels(steady), 2.0 },
        { "overshoot.db", "peak", "dB", ToDecibels(peak /
            std::max(steady * std::sqrt(2.0), kMinimumAmplitude)), 2.0 }
    };
    constexpr std::size_t kEnvelopePoints = 16;
    for (std::size_t point = 0; point < kEnvelopePoints; ++point)
    {
        const auto offset = static_cast<std::size_t>(std::round(
            static_cast<double>(tail) * static_cast<double>(point) /
            static_cast<double>(kEnvelopePoints - 1)));
        const auto index = std::min(release + offset, envelope.size() - 1);
        features.push_back({
            "release-envelope." + std::to_string(point) + ".dbfs",
            "envelope", "dBFS", ToDecibels(envelope[index]), 3.0
        });
    }
    return { "transient-burst", "3", std::move(features) };
}

} // namespace emulation_lab

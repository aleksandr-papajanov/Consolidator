#include "Probe.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>
#include <stdexcept>
#include <utility>
#include <vector>

namespace emulation_lab
{

namespace
{

constexpr double kMinimumAmplitude = 1.0e-15;

double ToDecibels(double value) noexcept
{
    return 20.0 * std::log10(std::max(value, kMinimumAmplitude));
}

std::complex<double> Projection(
    std::span<const double> samples,
    double frequency,
    double sampleRate)
{
    std::complex<double> sum{};
    auto windowSum = 0.0;
    const auto denominator = static_cast<double>(std::max<std::size_t>(samples.size() - 1, 1));
    for (std::size_t index = 0; index < samples.size(); ++index)
    {
        const auto window = 0.5 - 0.5 * std::cos(
            2.0 * std::numbers::pi * static_cast<double>(index) / denominator);
        const auto phase = -2.0 * std::numbers::pi * frequency *
            static_cast<double>(index) / sampleRate;
        sum += window * samples[index] * std::polar(1.0, phase);
        windowSum += window;
    }
    return windowSum > 0.0 ? 2.0 * sum / windowSum : std::complex<double>{};
}

ProbeResult Analyze(
    const LevelSweepConfiguration& configuration,
    double sampleRate,
    double levelDb,
    std::span<const double> output)
{
    auto sumSquares = 0.0;
    auto peak = 0.0;
    for (const auto sample : output)
    {
        sumSquares += sample * sample;
        peak = std::max(peak, std::abs(sample));
    }
    const auto fundamental = std::abs(Projection(output, configuration.frequency, sampleRate));
    auto harmonicEnergy = 0.0;
    for (std::size_t harmonic = 2; harmonic <= configuration.maximumHarmonic; ++harmonic)
    {
        if (configuration.frequency * static_cast<double>(harmonic) >= sampleRate * 0.5)
        {
            break;
        }
        const auto amplitude = std::abs(Projection(
            output, configuration.frequency * static_cast<double>(harmonic), sampleRate));
        harmonicEnergy += amplitude * amplitude;
    }
    const auto rms = std::sqrt(sumSquares /
        static_cast<double>(std::max<std::size_t>(output.size(), 1)));
    std::vector<ProbeFeature> features{
            { "gain.db", "gainDb", "dB", ToDecibels(fundamental) - levelDb, 1.0 },
            { "thd.dbc", "thd", "dBc", ToDecibels(std::sqrt(harmonicEnergy) /
                std::max(fundamental, kMinimumAmplitude)), 6.0, configuration.harmonicFloorDbc },
            { "crest.db", "peak", "dB", ToDecibels(peak /
                std::max(rms, kMinimumAmplitude)), 2.0 }
        };
    const auto windowSamples = static_cast<std::size_t>(std::round(0.020 * sampleRate));
    for (const auto centerMs : { 50, 150, 350, 750 })
    {
        const auto center = static_cast<std::size_t>(std::round(
            static_cast<double>(centerMs) * 0.001 * sampleRate));
        if (center < windowSamples / 2 || center + windowSamples / 2 > output.size())
        {
            continue;
        }
        auto windowEnergy = 0.0;
        for (auto index = center - windowSamples / 2;
            index < center - windowSamples / 2 + windowSamples; ++index)
        {
            windowEnergy += output[index] * output[index];
        }
        const auto windowRms = std::sqrt(windowEnergy / static_cast<double>(windowSamples));
        features.push_back({
            "gain." + std::to_string(centerMs) + "ms.db", "gainDb", "dB",
            ToDecibels(windowRms) - levelDb + 3.0102999566, 2.0
        });
    }
    return { "level-sweep", "3", std::move(features) };
}

} // namespace

LevelSweepProbe::LevelSweepProbe(LevelSweepConfiguration configuration)
    : configuration_(std::move(configuration))
{
}

std::vector<ProbeResult> LevelSweepProbe::Run(
    const ProbeContext& context,
    const ProcessorCallbacks& processor) const
{
    if (!processor.process || context.sampleRate <= 0.0)
    {
        throw std::invalid_argument("Level sweep needs a processor and a valid sample rate.");
    }
    if (processor.reset)
    {
        processor.reset();
    }
    const auto sampleCount = std::max<std::size_t>(
        static_cast<std::size_t>(std::ceil(configuration_.secondsPerLevel * context.sampleRate)), 2);
    if (configuration_.levelsDb.empty())
    {
        throw std::invalid_argument("Level sweep needs at least one level.");
    }
    const auto latency = processor.latencySamples ? processor.latencySamples() : 0;
    std::vector<double> input(sampleCount * configuration_.levelsDb.size() + latency);
    for (std::size_t levelIndex = 0;
        levelIndex < configuration_.levelsDb.size();
        ++levelIndex)
    {
        const auto levelDb = configuration_.levelsDb[levelIndex];
        const auto amplitude = std::pow(10.0, levelDb / 20.0);
        const auto start = levelIndex * sampleCount;
        for (std::size_t index = 0; index < sampleCount; ++index)
        {
            input[start + index] = amplitude * std::sin(
                2.0 * std::numbers::pi * configuration_.frequency *
                static_cast<double>(start + index) / context.sampleRate);
        }
    }
    const auto finalAmplitude = std::pow(10.0, configuration_.levelsDb.back() / 20.0);
    for (std::size_t index = input.size() - latency; index < input.size(); ++index)
    {
        input[index] = finalAmplitude * std::sin(
            2.0 * std::numbers::pi * configuration_.frequency *
            static_cast<double>(index) / context.sampleRate);
    }
    const auto output = processor.process(input);
    if (output.size() != input.size())
    {
        throw std::runtime_error("Processor returned a short level-sweep buffer.");
    }
    std::vector<ProbeResult> results;
    for (std::size_t levelIndex = 0;
        levelIndex < configuration_.levelsDb.size();
        ++levelIndex)
    {
        const auto start = levelIndex * sampleCount + latency;
        results.push_back(Analyze(
            configuration_, context.sampleRate, configuration_.levelsDb[levelIndex],
            std::span<const double>(output.data() + start, sampleCount)));
    }
    return results;
}

} // namespace emulation_lab

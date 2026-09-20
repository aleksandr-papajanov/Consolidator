#include "Probe.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace emulation_lab
{

namespace
{

constexpr double kMinimumAmplitude = 1.0e-15;

double ToDecibels(double value) noexcept
{
    return 20.0 * std::log10(std::max(value, kMinimumAmplitude));
}

} // namespace

MultitoneProbe::MultitoneProbe(MultitoneConfiguration configuration)
    : configuration_(std::move(configuration))
{
}

ProbeResult MultitoneProbe::Run(
    const ProbeContext& context,
    const ProcessorCallbacks& processor) const
{
    if (!processor.process || context.sampleRate <= 0.0 || configuration_.frequencies.empty())
    {
        throw std::invalid_argument("Multitone probe needs tones, a processor and a valid sample rate.");
    }
    if (processor.reset)
    {
        processor.reset();
    }
    const auto sampleCount = std::max<std::size_t>(configuration_.samples, 2);
    const auto latency = processor.latencySamples ? processor.latencySamples() : 0;
    std::vector<double> coherentFrequencies;
    coherentFrequencies.reserve(configuration_.frequencies.size());
    std::unordered_set<std::size_t> coherentBins;
    for (const auto requested : configuration_.frequencies)
    {
        const auto bin = static_cast<std::size_t>(std::max(1.0, std::round(requested *
            static_cast<double>(sampleCount) / context.sampleRate)));
        if (bin >= sampleCount / 2 || !coherentBins.insert(bin).second)
        {
            throw std::invalid_argument(
                "Multitone frequencies alias or collapse onto the same coherent bin.");
        }
        coherentFrequencies.push_back(static_cast<double>(bin) * context.sampleRate /
            static_cast<double>(sampleCount));
    }
    const auto amplitude = std::pow(10.0, configuration_.levelDb / 20.0) /
        static_cast<double>(coherentFrequencies.size());
    std::vector<double> input(sampleCount + latency);
    for (std::size_t index = 0; index < input.size(); ++index)
    {
        for (const auto frequency : coherentFrequencies)
        {
            input[index] += amplitude * std::sin(2.0 * std::numbers::pi * frequency *
                static_cast<double>(index) / context.sampleRate);
        }
    }
    const auto output = processor.process(input);
    if (output.size() != input.size())
    {
        throw std::runtime_error("Processor returned a short multitone buffer.");
    }
    const auto measuredOutput = std::span<const double>(output.data() + latency, sampleCount);
    std::vector<double> residual(measuredOutput.begin(), measuredOutput.end());
    auto primaryEnergy = 0.0;
    for (const auto frequency : coherentFrequencies)
    {
        auto sine = 0.0;
        auto cosine = 0.0;
        for (std::size_t index = 0; index < measuredOutput.size(); ++index)
        {
            const auto phase = 2.0 * std::numbers::pi * frequency *
                static_cast<double>(index) / context.sampleRate;
            sine += measuredOutput[index] * std::sin(phase);
            cosine += measuredOutput[index] * std::cos(phase);
        }
        sine *= 2.0 / static_cast<double>(measuredOutput.size());
        cosine *= 2.0 / static_cast<double>(measuredOutput.size());
        primaryEnergy += 0.5 * (sine * sine + cosine * cosine);
        for (std::size_t index = 0; index < residual.size(); ++index)
        {
            const auto phase = 2.0 * std::numbers::pi * frequency *
                static_cast<double>(index) / context.sampleRate;
            residual[index] -= sine * std::sin(phase) + cosine * std::cos(phase);
        }
    }
    auto residualEnergy = 0.0;
    for (const auto sample : residual)
    {
        residualEnergy += sample * sample;
    }
    residualEnergy /= static_cast<double>(residual.size());
    const auto primaryRms = std::sqrt(std::max(primaryEnergy, 0.0));
    const auto residualRms = std::sqrt(std::max(residualEnergy, 0.0));
    return {
        "multitone", "2",
        {
            { "primary.dbfs", "gainDb", "dBFS", ToDecibels(primaryRms), 1.0 },
            { "residual.dbfs", "intermodulation", "dBFS", ToDecibels(residualRms), 6.0, -180.0 },
            { "imd.dbc", "intermodulation", "dBc", ToDecibels(residualRms /
                std::max(primaryRms, kMinimumAmplitude)), 6.0, -180.0 }
        }
    };
}

} // namespace emulation_lab

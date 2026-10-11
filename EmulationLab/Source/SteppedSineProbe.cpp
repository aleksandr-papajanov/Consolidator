#include "Probe.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <numbers>
#include <stdexcept>
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

double WrapDegrees(double value) noexcept
{
    return std::remainder(value, 360.0);
}

std::complex<double> MeasureFrequency(
    std::span<const double> samples,
    double sampleRate,
    double frequency)
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
    const ToneCase& tone,
    double sampleRate,
    std::size_t maximumHarmonic,
    double harmonicFloorDbc,
    std::span<const double> input,
    std::span<const double> output)
{
    if (input.size() != output.size() || input.size() < 2)
    {
        throw std::runtime_error("Stepped-sine input and output windows differ.");
    }

    auto sumSquares = 0.0;
    auto peak = 0.0;
    for (const auto sample : output)
    {
        sumSquares += sample * sample;
        peak = std::max(peak, std::abs(sample));
    }

    const auto inputFundamental = MeasureFrequency(input, sampleRate, tone.frequency);
    const auto outputFundamental = MeasureFrequency(output, sampleRate, tone.frequency);
    const auto fundamentalAmplitude = std::abs(outputFundamental);
    auto harmonicEnergy = 0.0;
    std::vector<ProbeFeature> features{
        { "gain.db", "gainDb", "dB", ToDecibels(fundamentalAmplitude) -
            ToDecibels(std::abs(inputFundamental)), 1.0 },
        { "phase.degrees", "phase", "degrees", WrapDegrees(
            std::arg(outputFundamental) * 180.0 / std::numbers::pi -
            std::arg(inputFundamental) * 180.0 / std::numbers::pi),
            30.0, -std::numeric_limits<double>::infinity(), FeatureComparison::WrappedDegrees },
        { "rms.dbfs", "rms", "dBFS", ToDecibels(std::sqrt(
            sumSquares / static_cast<double>(output.size()))), 2.0 },
        { "peak.dbfs", "peak", "dBFS", ToDecibels(peak), 2.0 }
    };

    for (std::size_t harmonic = 2; harmonic <= maximumHarmonic; ++harmonic)
    {
        const auto harmonicFrequency = tone.frequency * static_cast<double>(harmonic);
        if (harmonicFrequency >= sampleRate * 0.5)
        {
            break;
        }
        const auto projection = MeasureFrequency(output, sampleRate, harmonicFrequency);
        const auto amplitude = std::abs(projection);
        const auto relative = projection / std::max(fundamentalAmplitude, kMinimumAmplitude) *
            std::polar(1.0, -static_cast<double>(harmonic) * std::arg(inputFundamental));
        harmonicEnergy += amplitude * amplitude;
        features.push_back({
            "harmonic." + std::to_string(harmonic) + ".dbc",
            "harmonics", "dBc",
            ToDecibels(amplitude / std::max(fundamentalAmplitude, kMinimumAmplitude)),
            6.0, harmonicFloorDbc
        });
        features.push_back({
            "harmonic." + std::to_string(harmonic) + ".real",
            "harmonics", "ratio", relative.real(), 0.01
        });
        features.push_back({
            "harmonic." + std::to_string(harmonic) + ".imag",
            "harmonics", "ratio", relative.imag(), 0.01
        });
    }
    features.push_back({
        "thd.dbc", "thd", "dBc",
        ToDecibels(std::sqrt(harmonicEnergy) /
            std::max(fundamentalAmplitude, kMinimumAmplitude)),
        6.0, harmonicFloorDbc
    });
    return { "stepped-sine", "3", std::move(features) };
}

} // namespace

SteppedSineProbe::SteppedSineProbe(SteppedSineConfiguration configuration)
    : configuration_(configuration)
{
}

std::vector<ToneCase> SteppedSineProbe::Cases() const
{
    std::vector<ToneCase> result;
    if (configuration_.minimumFrequency <= 0.0 ||
        configuration_.maximumFrequency < configuration_.minimumFrequency ||
        configuration_.pointsPerOctave <= 0.0 || configuration_.levelStepDb <= 0.0)
    {
        return result;
    }
    for (auto level = configuration_.minimumLevelDb;
        level <= configuration_.maximumLevelDb + 1.0e-9;
        level += configuration_.levelStepDb)
    {
        for (auto octavePoint = 0.0;; octavePoint += 1.0)
        {
            const auto frequency = configuration_.minimumFrequency *
                std::pow(2.0, octavePoint / configuration_.pointsPerOctave);
            if (frequency > configuration_.maximumFrequency)
            {
                break;
            }
            result.push_back({ frequency, level });
        }
    }
    return result;
}

std::vector<ProbeResult> SteppedSineProbe::Run(
    const ProbeContext& context,
    const ProcessorCallbacks& processor) const
{
    if (!processor.process || context.sampleRate <= 0.0)
    {
        throw std::invalid_argument("Stepped-sine needs a processor and a valid sample rate.");
    }
    std::vector<ProbeResult> results;
    for (const auto& tone : Cases())
    {
        results.push_back(RunTone(context, processor, tone));
    }
    return results;
}

ProbeResult SteppedSineProbe::RunTone(
    const ProbeContext& context,
    const ProcessorCallbacks& processor,
    const ToneCase& tone) const
{
    if (!processor.process || context.sampleRate <= 0.0 || tone.frequency <= 0.0)
    {
        throw std::invalid_argument("Stepped-sine tone is invalid.");
    }
    if (configuration_.resetEachPoint && processor.reset)
    {
        processor.reset();
    }
    const auto latency = processor.latencySamples ? processor.latencySamples() : 0;
    const auto preRollSamples = static_cast<std::size_t>(std::ceil(std::max(
        configuration_.minimumPreRollSeconds,
        configuration_.preRollCycles / tone.frequency) * context.sampleRate));
    const auto measurementSamples = static_cast<std::size_t>(std::ceil(std::max(
        configuration_.minimumMeasurementSeconds,
        configuration_.measurementCycles / tone.frequency) * context.sampleRate));
    const auto totalSamples = preRollSamples + measurementSamples + latency;
    const auto amplitude = std::pow(10.0, tone.levelDb / 20.0);
    std::vector<double> input(totalSamples);
    for (std::size_t index = 0; index < input.size(); ++index)
    {
        input[index] = amplitude * std::sin(2.0 * std::numbers::pi * tone.frequency *
            static_cast<double>(index) / context.sampleRate);
    }
    const auto output = processor.process(input);
    if (output.size() < preRollSamples + latency + measurementSamples)
    {
        throw std::runtime_error("Processor returned a short stepped-sine buffer.");
    }
    return Analyze(
        tone, context.sampleRate, configuration_.maximumHarmonic,
        configuration_.harmonicFloorDbc,
        std::span<const double>(input.data() + preRollSamples, measurementSamples),
        std::span<const double>(
            output.data() + preRollSamples + latency, measurementSamples));
}

} // namespace emulation_lab

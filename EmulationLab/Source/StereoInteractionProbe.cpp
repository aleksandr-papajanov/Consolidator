#include "Probe.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>
#include <stdexcept>
#include <utility>

namespace emulation_lab
{

namespace
{

constexpr double kMinimumAmplitude = 1.0e-15;

double Amplitude(std::span<const double> samples, double frequency, double sampleRate)
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
    return windowSum > 0.0 ? std::abs(2.0 * sum / windowSum) : 0.0;
}

double ToDecibels(double value) noexcept
{
    return 20.0 * std::log10(std::max(value, kMinimumAmplitude));
}

} // namespace

StereoInteractionProbe::StereoInteractionProbe(
    StereoInteractionConfiguration configuration)
    : configuration_(std::move(configuration))
{
}

ProbeResult StereoInteractionProbe::Run(
    const ProbeContext& context,
    const ProcessorCallbacks& processor) const
{
    if (!processor.processStereo || context.sampleRate <= 0.0)
    {
        throw std::invalid_argument("Stereo probe needs a stereo processor callback.");
    }
    const auto latency = processor.latencySamples ? processor.latencySamples() : 0;
    const auto preRoll = static_cast<std::size_t>(std::ceil(0.25 * context.sampleRate));
    const auto measured = static_cast<std::size_t>(std::ceil(
        std::max(configuration_.seconds, 20.0 / configuration_.frequency) * context.sampleRate));
    const auto total = preRoll + measured + latency;
    const auto inputAmplitude = std::pow(10.0, configuration_.levelDb / 20.0);
    std::vector<double> tone(total);
    std::vector<double> silence(total);
    for (std::size_t index = 0; index < tone.size(); ++index)
    {
        tone[index] = inputAmplitude * std::sin(2.0 * std::numbers::pi *
            configuration_.frequency * static_cast<double>(index) / context.sampleRate);
    }
    if (processor.reset)
    {
        processor.reset();
    }
    const auto leftDriven = processor.processStereo(tone, silence);
    if (processor.reset)
    {
        processor.reset();
    }
    const auto rightDriven = processor.processStereo(silence, tone);
    const auto start = preRoll + latency;
    if (leftDriven.left.size() < start + measured ||
        leftDriven.right.size() < start + measured ||
        rightDriven.left.size() < start + measured ||
        rightDriven.right.size() < start + measured)
    {
        throw std::runtime_error("Processor returned a short stereo buffer.");
    }
    const auto leftMain = Amplitude(
        std::span<const double>(leftDriven.left.data() + start, measured),
        configuration_.frequency, context.sampleRate);
    const auto leftLeak = Amplitude(
        std::span<const double>(leftDriven.right.data() + start, measured),
        configuration_.frequency, context.sampleRate);
    const auto rightMain = Amplitude(
        std::span<const double>(rightDriven.right.data() + start, measured),
        configuration_.frequency, context.sampleRate);
    const auto rightLeak = Amplitude(
        std::span<const double>(rightDriven.left.data() + start, measured),
        configuration_.frequency, context.sampleRate);
    return {
        "stereo-interaction", "1",
        {
            { "left-gain.db", "gainDb", "dB", ToDecibels(leftMain / inputAmplitude), 1.0 },
            { "right-gain.db", "gainDb", "dB", ToDecibels(rightMain / inputAmplitude), 1.0 },
            { "left-to-right.dbc", "stereo", "dBc", ToDecibels(leftLeak /
                std::max(leftMain, kMinimumAmplitude)), 6.0, -180.0 },
            { "right-to-left.dbc", "stereo", "dBc", ToDecibels(rightLeak /
                std::max(rightMain, kMinimumAmplitude)), 6.0, -180.0 }
        }
    };
}

} // namespace emulation_lab

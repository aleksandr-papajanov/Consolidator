#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "Dsp/PolishAirProcessor.h"

namespace consolidator::tests
{

namespace
{

constexpr std::size_t kFrameCount = 4410;
constexpr double kSampleRate = 44100.0;
constexpr double kPi = 3.14159265358979323846;

void MakeSine(
    std::array<double, kFrameCount>& samples,
    double frequency,
    double amplitude)
{
    for (std::size_t index = 0; index < samples.size(); ++index)
    {
        samples[index] = amplitude * std::sin(
            2.0 * kPi * frequency *
            static_cast<double>(index) / kSampleRate);
    }
}

double ToneMagnitude(
    const std::array<double, kFrameCount>& samples,
    double frequency)
{
    auto real = 0.0;
    auto imaginary = 0.0;
    for (std::size_t index = 0; index < samples.size(); ++index)
    {
        const auto phase = 2.0 * kPi * frequency *
            static_cast<double>(index) / kSampleRate;
        real += samples[index] * std::cos(phase);
        imaginary -= samples[index] * std::sin(phase);
    }
    return 2.0 * std::sqrt(real * real + imaginary * imaginary) /
        static_cast<double>(samples.size());
}

double ToDecibels(double amplitude)
{
    return 20.0 * std::log10(std::max(amplitude, 1.0e-15));
}

std::array<double, kFrameCount> RenderSettledTone(
    double frequency,
    double amplitude)
{
    using namespace consolidator::max::dsp;

    PolishAirProcessor processor;
    processor.Prepare({ kSampleRate, kFrameCount });
    DspSnapshot snapshot{};
    snapshot.polishAir = 1.0F;
    processor.SetParameters(snapshot);
    std::array<double, kFrameCount> left{};
    std::array<double, kFrameCount> right{};
    DspProcessingPlan plan{};
    DspExecutionPolicy policy{};
    DspProcessingFrame frame{
        { left.data(), right.data(), left.size() },
        {},
        plan,
        policy
    };
    for (auto block = 0; block < 20; ++block)
    {
        MakeSine(left, frequency, amplitude);
        right = left;
        processor.Process(frame);
    }
    return left;
}

} // namespace

bool RunPolishAirProcessorTests()
{
    using namespace consolidator::max::dsp;

    auto succeeded = true;
    PolishAirProcessor processor;
    processor.Prepare({ kSampleRate, kFrameCount });
    DspSnapshot snapshot{};
    processor.SetParameters(snapshot);
    std::array<double, kFrameCount> left{};
    std::array<double, kFrameCount> right{};
    MakeSine(left, 4000.0, 0.5);
    right = left;
    const auto original = left;
    DspProcessingPlan plan{};
    DspExecutionPolicy policy{};
    DspProcessingFrame frame{
        { left.data(), right.data(), left.size() },
        {},
        plan,
        policy
    };
    processor.Process(frame);
    succeeded &= Expect(
        left == original && right == original,
        "Air changed audio at zero amount.");

    const auto lowFrequency = RenderSettledTone(1000.0, 0.001);
    const auto highFrequency = RenderSettledTone(18000.0, 0.001);
    const auto lowFrequencyGain = ToneMagnitude(lowFrequency, 1000.0) / 0.001;
    const auto highFrequencyGain = ToneMagnitude(highFrequency, 18000.0) / 0.001;
    succeeded &= Expect(
        highFrequencyGain > lowFrequencyGain * 1.3,
        "Air did not preserve the measured high-frequency lift.");

    const auto quietHighFrequency = RenderSettledTone(18000.0, 0.001);
    const auto loudHighFrequency = RenderSettledTone(18000.0, 1.0);
    const auto quietGain = ToneMagnitude(quietHighFrequency, 18000.0) / 0.001;
    const auto loudGain = ToneMagnitude(loudHighFrequency, 18000.0);
    succeeded &= Expect(
        std::abs(ToDecibels(quietGain) - 11.1) < 0.25,
        "Air quiet 18 kHz lift no longer matches the calibrated reference.");
    succeeded &= Expect(
        std::abs(ToDecibels(loudGain) - 8.51) < 0.35,
        "Air loud 18 kHz lift no longer matches the calibrated reference.");

    const auto nonlinear = RenderSettledTone(4000.0, 1.0);
    const auto fundamental = ToneMagnitude(nonlinear, 4000.0);
    const auto secondHarmonic = ToneMagnitude(nonlinear, 8000.0);
    const auto thirdHarmonic = ToneMagnitude(nonlinear, 12000.0);
    succeeded &= Expect(
        thirdHarmonic > secondHarmonic * 2.0,
        "Air did not preserve the measured odd-dominant harmonic balance.");
    succeeded &= Expect(
        std::abs(ToDecibels(thirdHarmonic / fundamental) + 50.1) < 1.2,
        "Air H3 no longer matches the calibrated 4 kHz reference.");
    succeeded &= Expect(
        std::isfinite(quietGain) && std::isfinite(loudGain) &&
            std::isfinite(fundamental) && std::isfinite(secondHarmonic) &&
            std::isfinite(thirdHarmonic),
        "Air produced a non-finite response.");
    return succeeded;
}

} // namespace consolidator::tests

#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "Dsp/PolishThickProcessor.h"

namespace consolidator::tests
{

namespace
{

constexpr std::size_t kFrameCount = 1470;
constexpr std::size_t kLinearFrameCount = 4410;
constexpr double kSampleRate = 44100.0;
constexpr double kFrequency = 60.0;
constexpr double kPi = 3.14159265358979323846;

template<std::size_t SampleCount>
void MakeSine(
    std::array<double, SampleCount>& samples,
    double frequency = kFrequency,
    double amplitude = 1.0)
{
    for (std::size_t index = 0; index < samples.size(); ++index)
    {
        samples[index] = amplitude * std::sin(
            2.0 * kPi * frequency *
            static_cast<double>(index) / kSampleRate);
    }
}

template<std::size_t SampleCount>
double HarmonicMagnitude(
    const std::array<double, SampleCount>& output,
    const std::array<double, SampleCount>& input,
    double harmonic)
{
    auto real = 0.0;
    auto imaginary = 0.0;
    for (std::size_t index = 0; index < output.size(); ++index)
    {
        const auto wet = output[index] - input[index];
        const auto phase = 2.0 * kPi * kFrequency * harmonic *
            static_cast<double>(index) / kSampleRate;
        real += wet * std::cos(phase);
        imaginary -= wet * std::sin(phase);
    }
    return 2.0 * std::sqrt(real * real + imaginary * imaginary) /
        static_cast<double>(output.size());
}

template<std::size_t SampleCount>
double ToneMagnitude(
    const std::array<double, SampleCount>& samples,
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

std::array<double, kLinearFrameCount> RenderSettledTone(
    double frequency,
    double amplitude,
    const consolidator::max::dsp::PolishThickTuning* tuning = nullptr)
{
    using namespace consolidator::max::dsp;

    constexpr std::size_t kProcessingBlockSize = 128;
    PolishThickProcessor processor;
    if (tuning != nullptr)
    {
        processor.SetTuningBeforePrepare(*tuning);
    }
    processor.Prepare({ kSampleRate, kProcessingBlockSize });
    DspSnapshot snapshot{};
    snapshot.polishThick = 1.0F;
    processor.SetParameters(snapshot);
    std::array<double, kProcessingBlockSize> left{};
    std::array<double, kProcessingBlockSize> right{};
    std::array<double, kLinearFrameCount> result{};
    DspProcessingPlan plan{};
    DspExecutionPolicy policy{};
    std::size_t sourceIndex = 0;
    std::size_t resultIndex = 0;
    const auto process = [&](std::size_t frameCount, bool capture)
    {
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            left[index] = amplitude * std::sin(
                2.0 * kPi * frequency *
                static_cast<double>(sourceIndex++) / kSampleRate);
            right[index] = left[index];
        }
        DspProcessingFrame frame{
            { left.data(), right.data(), frameCount },
            {},
            plan,
            policy };
        processor.Process(frame);
        if (capture)
        {
            std::copy_n(left.begin(), frameCount, result.begin() + resultIndex);
            resultIndex += frameCount;
        }
    };
    for (std::size_t remaining = static_cast<std::size_t>(kSampleRate);
        remaining > 0;)
    {
        const auto frameCount = std::min(remaining, kProcessingBlockSize);
        process(frameCount, false);
        remaining -= frameCount;
    }
    while (resultIndex < result.size())
    {
        process(std::min(result.size() - resultIndex, kProcessingBlockSize), true);
    }
    return result;
}

struct TransientMeasurement
{
    double attackPeak{};
    double attackRootMeanSquare{};
    double tailRootMeanSquare{};
};

TransientMeasurement MeasureKickTransient(
    const consolidator::max::dsp::PolishThickTuning& tuning)
{
    using namespace consolidator::max::dsp;

    constexpr std::size_t kProcessingBlockSize = 128;
    constexpr std::size_t kMeasurementSamples = 17640;
    constexpr std::size_t kAttackSamples = 2205;
    constexpr std::size_t kTailStart = 13230;
    PolishThickProcessor processor;
    processor.SetTuningBeforePrepare(tuning);
    processor.Prepare({ kSampleRate, kProcessingBlockSize });
    DspSnapshot snapshot{};
    snapshot.polishThick = 1.0F;
    processor.SetParameters(snapshot);
    DspProcessingPlan plan{};
    DspExecutionPolicy policy{};
    std::array<double, kProcessingBlockSize> left{};
    std::array<double, kProcessingBlockSize> right{};
    std::size_t sourceIndex = 0;
    const auto fillBed = [&](std::size_t frameCount)
    {
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            const auto time = static_cast<double>(sourceIndex++) / kSampleRate;
            left[index] = 0.12 * std::sin(2.0 * kPi * 80.0 * time) +
                0.08 * std::sin(2.0 * kPi * 500.0 * time);
            right[index] = left[index];
        }
    };
    for (std::size_t remaining = static_cast<std::size_t>(kSampleRate);
        remaining > 0;)
    {
        const auto frameCount = std::min(remaining, kProcessingBlockSize);
        fillBed(frameCount);
        DspProcessingFrame frame{
            { left.data(), right.data(), frameCount },
            {},
            plan,
            policy };
        processor.Process(frame);
        remaining -= frameCount;
    }

    TransientMeasurement measurement;
    auto attackEnergy = 0.0;
    auto tailEnergy = 0.0;
    auto tailSamples = 0U;
    for (std::size_t position = 0; position < kMeasurementSamples;)
    {
        const auto frameCount = std::min(
            kMeasurementSamples - position,
            kProcessingBlockSize);
        fillBed(frameCount);
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            const auto time = static_cast<double>(position + index) /
                kSampleRate;
            const auto kick = 0.95 * std::exp(-time / 0.050) *
                std::sin(2.0 * kPi * 60.0 * time) +
                0.5 * std::exp(-time / 0.003);
            left[index] += kick;
            right[index] += kick;
        }
        DspProcessingFrame frame{
            { left.data(), right.data(), frameCount },
            {},
            plan,
            policy };
        processor.Process(frame);
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            const auto measurementIndex = position + index;
            if (measurementIndex < kAttackSamples)
            {
                measurement.attackPeak = std::max(
                    measurement.attackPeak,
                    std::abs(left[index]));
                attackEnergy += left[index] * left[index];
            }
            if (measurementIndex >= kTailStart)
            {
                tailEnergy += left[index] * left[index];
                ++tailSamples;
            }
        }
        position += frameCount;
    }
    measurement.attackRootMeanSquare = std::sqrt(
        attackEnergy / static_cast<double>(kAttackSamples));
    measurement.tailRootMeanSquare = std::sqrt(
        tailEnergy / static_cast<double>(tailSamples));
    return measurement;
}

double MeasureLinearGain(
    double frequency,
    double amplitude = 0.001,
    const consolidator::max::dsp::PolishThickTuning* tuning = nullptr)
{
    using namespace consolidator::max::dsp;

    PolishThickProcessor processor;
    if (tuning != nullptr)
    {
        processor.SetTuningBeforePrepare(*tuning);
    }
    processor.Prepare({ kSampleRate, kLinearFrameCount });
    DspSnapshot snapshot{};
    snapshot.polishThick = 1.0F;
    processor.SetParameters(snapshot);
    std::array<double, kLinearFrameCount> left{};
    std::array<double, kLinearFrameCount> right{};
    DspProcessingPlan plan{};
    DspExecutionPolicy policy{};
    DspProcessingFrame frame{
        { left.data(), right.data(), left.size() },
        {},
        plan,
        policy };
    for (auto block = 0; block < 40; ++block)
    {
        MakeSine(left, frequency, amplitude);
        right = left;
        processor.Process(frame);
    }
    return ToneMagnitude(left, frequency) / amplitude;
}

double MeasureFirstBlockLinearGain(double frequency, double amplitude)
{
    using namespace consolidator::max::dsp;

    PolishThickProcessor processor;
    processor.Prepare({ kSampleRate, kFrameCount });
    DspSnapshot snapshot{};
    snapshot.polishThick = 1.0F;
    processor.SetParameters(snapshot);
    std::array<double, kFrameCount> left{};
    std::array<double, kFrameCount> right{};
    DspProcessingPlan plan{};
    DspExecutionPolicy policy{};
    DspProcessingFrame frame{
        { left.data(), right.data(), left.size() },
        {},
        plan,
        policy };
    processor.Process(frame);
    MakeSine(left, frequency, amplitude);
    right = left;
    processor.Process(frame);
    return ToneMagnitude(left, frequency) / amplitude;
}

}

bool RunPolishThickProcessorTests()
{
    using namespace consolidator::max::dsp;

    auto succeeded = true;
    PolishThickProcessor processor;
    processor.Prepare({ kSampleRate, kFrameCount });

    DspProcessingPlan plan{};
    DspExecutionPolicy policy{};
    DspSnapshot snapshot{};

    std::array<double, kFrameCount> left{};
    std::array<double, kFrameCount> right{};
    MakeSine(left);
    right = left;
    const auto original = left;
    DspProcessingFrame frame{
        { left.data(), right.data(), left.size() },
        {},
        plan,
        policy };
    processor.SetParameters(snapshot);
    processor.Process(frame);
    succeeded &= Expect(
        left == original && right == original,
        "Thick changed audio at zero amount.");

    snapshot.polishThick = 1.0F;
    processor.SetParameters(snapshot);
    for (auto block = 0; block < 40; ++block)
    {
        MakeSine(left);
        right = left;
        processor.Process(frame);
    }

    auto difference = 0.0;
    auto energy = 0.0;
    for (std::size_t index = 0; index < left.size(); ++index)
    {
        difference += std::abs(left[index] - original[index]);
        energy += left[index] * left[index];
        succeeded &= Expect(
            std::isfinite(left[index]) && left[index] == right[index],
            "Thick produced non-finite or asymmetric stereo output.");
    }

    succeeded &= Expect(
        difference > 1.0e-4 && energy > 0.0,
        "Thick produced no audible response at maximum amount.");
    succeeded &= Expect(
        ToneMagnitude(left, kFrequency) > 0.75,
        "Thick body branch did not produce the documented low-frequency lift.");
    const auto gainAt20 = MeasureLinearGain(20.0);
    const auto gainAt30 = MeasureLinearGain(30.0);
    const auto gainAt40 = MeasureLinearGain(40.0);
    const auto gainAt60 = MeasureLinearGain(60.0);
    const auto gainAt80 = MeasureLinearGain(80.0);
    const auto gainAt100 = MeasureLinearGain(100.0);
    const auto gainAt200 = MeasureLinearGain(200.0);
    succeeded &= Expect(
        gainAt30 < gainAt20 && gainAt30 < gainAt40 &&
            gainAt80 > gainAt60 && gainAt80 > gainAt100 &&
            gainAt100 > gainAt200,
        "Thick linear response did not preserve the measured notch and body peak.");
    const auto lowLevelBodyGain = MeasureLinearGain(80.0, 0.03162277660168379);
    const auto highLevelBodyGain = MeasureLinearGain(80.0, 0.5011872336272722);
    succeeded &= Expect(
        lowLevelBodyGain > highLevelBodyGain * 1.5,
        "Thick body mix did not decrease as the input approached 0 dBFS.");
    const auto positiveLevelGain = MeasureLinearGain(5000.0, 2.0);
    succeeded &= Expect(
        positiveLevelGain > 0.58 && positiveLevelGain < 0.64,
        "Thick did not apply the measured positive-level output gain.");
    const PolishThickTuning defaultTuning;
    succeeded &= Expect(
        std::abs(positiveLevelGain -
            MeasureLinearGain(5000.0, 2.0, &defaultTuning)) < 1.0e-12,
        "Explicit default Thick tuning changed the production response.");
    const auto firstBlockPositiveLevelGain = MeasureFirstBlockLinearGain(
        5000.0,
        2.0);
    succeeded &= Expect(
        firstBlockPositiveLevelGain > 0.58 &&
            firstBlockPositiveLevelGain < 0.64,
        "Thick changed a high-frequency first block outside the bass crest path.");
    constexpr double kPositiveThreeDbAmplitude = 1.4125375446227544;
    const auto positiveTone = RenderSettledTone(
        1000.0,
        kPositiveThreeDbAmplitude);
    const auto positivePeak = *std::max_element(
        positiveTone.begin(),
        positiveTone.end());
    const auto positiveThird = ToneMagnitude(positiveTone, 3000.0);
    succeeded &= Expect(
        positivePeak <= 1.0 + 1.0e-12,
        "Thick did not enforce the measured zero-dBFS ceiling.");
    succeeded &= Expect(
        std::abs(ToDecibels(positiveThird) + 16.47) < 0.6,
        "Thick positive-level clipping did not produce the measured third harmonic.");
    constexpr double kMinusThreeDbAmplitude = 0.7079457843841379;
    const auto subTone = RenderSettledTone(60.0, kMinusThreeDbAmplitude);
    const auto subSecond = ToneMagnitude(subTone, 120.0);
    const auto subThird = ToneMagnitude(subTone, 180.0);
    const auto subFourth = ToneMagnitude(subTone, 240.0);
    const auto subFifth = ToneMagnitude(subTone, 300.0);
    succeeded &= Expect(
        std::abs(ToDecibels(subSecond) + 20.38) < 1.5,
        "Thick weak mode did not preserve the measured sub second harmonic.");
    succeeded &= Expect(
        ToDecibels(subThird) > -35.0 &&
            ToDecibels(subFourth) > -48.0 &&
            ToDecibels(subFifth) > -44.0,
        "Thick weak mode omitted the measured sub harmonic orders three through five.");
    PolishThickTuning uncontrolledTransientTuning;
    uncontrolledTransientTuning.transientMinimumGain = 1.0;
    const auto uncontrolledKick = MeasureKickTransient(
        uncontrolledTransientTuning);
    const auto controlledKick = MeasureKickTransient(defaultTuning);
    succeeded &= Expect(
        controlledKick.attackRootMeanSquare <
            uncontrolledKick.attackRootMeanSquare * 0.95,
        "Thick crest control did not contain the kick enhancement.");
    succeeded &= Expect(
        controlledKick.tailRootMeanSquare >
            uncontrolledKick.tailRootMeanSquare * 0.9 &&
            controlledKick.tailRootMeanSquare <
                uncontrolledKick.tailRootMeanSquare * 1.1,
        "Thick crest control did not recover after the kick attack.");
    constexpr double kPositiveTenDbAmplitude = 3.1622776601683795;
    const auto positive30Hz = MeasureLinearGain(30.0, kPositiveTenDbAmplitude);
    const auto positive80Hz = MeasureLinearGain(80.0, kPositiveTenDbAmplitude);
    const auto positive100Hz = MeasureLinearGain(100.0, kPositiveTenDbAmplitude);
    const auto positive5000Hz = MeasureLinearGain(5000.0, kPositiveTenDbAmplitude);
    succeeded &= Expect(
        positive5000Hz > 0.38 && positive5000Hz < 0.42 &&
            positive30Hz < 0.35 && positive80Hz < 0.45 &&
            positive100Hz < 0.45,
        "Thick positive-level ceiling did not preserve the measured frequency response.");
    PolishThickTuning denseTuning;
    denseTuning.subHarmonicDrive = 0.0;
    const auto denseTone = RenderSettledTone(60.0, 1.0, &denseTuning);
    std::array<double, kLinearFrameCount> denseInput{};
    MakeSine(denseInput);
    succeeded &= Expect(
        HarmonicMagnitude(denseTone, denseInput, 5.0) > 1.0e-5 &&
            HarmonicMagnitude(denseTone, denseInput, 6.0) > 1.0e-5,
        "Thick did not produce the extended harmonic series.");
    succeeded &= Expect(
        HarmonicMagnitude(denseTone, denseInput, 12.0) > 1.0e-6 &&
            HarmonicMagnitude(denseTone, denseInput, 16.0) > 1.0e-6 &&
            HarmonicMagnitude(denseTone, denseInput, 20.0) > 1.0e-6,
        "Thick harmonic series terminated before the documented upper orders.");
    const auto oddHarmonics = HarmonicMagnitude(denseTone, denseInput, 3.0) +
        HarmonicMagnitude(denseTone, denseInput, 5.0) +
        HarmonicMagnitude(denseTone, denseInput, 7.0);
    const auto evenHarmonics = HarmonicMagnitude(denseTone, denseInput, 2.0) +
        HarmonicMagnitude(denseTone, denseInput, 4.0) +
        HarmonicMagnitude(denseTone, denseInput, 6.0);
    succeeded &= Expect(
        oddHarmonics > evenHarmonics * 4.0,
        "Thick did not preserve the documented odd-dominant harmonic balance.");
    const auto lowOddHarmonic = HarmonicMagnitude(denseTone, denseInput, 5.0);
    const auto highOddHarmonic = HarmonicMagnitude(denseTone, denseInput, 63.0);
    succeeded &= Expect(
        highOddHarmonic > lowOddHarmonic * 0.001,
        "Thick upper odd harmonics disappeared from the measured spectral shelf.");
    const auto highShelfHarmonic = HarmonicMagnitude(denseTone, denseInput, 151.0);
    succeeded &= Expect(
        highShelfHarmonic > lowOddHarmonic * 0.001,
        "Thick did not retain its high-frequency harmonic shelf.");
    succeeded &= Expect(
        highShelfHarmonic < lowOddHarmonic * 0.3,
        "Thick high-order harmonic cloud did not decay.");

    constexpr double kAnalyzerFrequency = 990.0;
    constexpr double kLowLevelAmplitude = 0.35481338923357547;
    PolishThickProcessor highFrequencyProcessor;
    highFrequencyProcessor.Prepare({ kSampleRate, kFrameCount });
    highFrequencyProcessor.SetParameters(snapshot);
    std::array<double, kFrameCount> highLeft{};
    std::array<double, kFrameCount> highRight{};
    MakeSine(highLeft, kAnalyzerFrequency);
    highRight = highLeft;
    DspProcessingFrame highFrame{
        { highLeft.data(), highRight.data(), highLeft.size() },
        {},
        plan,
        policy };
    for (auto block = 0; block < 40; ++block)
    {
        MakeSine(highLeft, kAnalyzerFrequency);
        highRight = highLeft;
        highFrequencyProcessor.Process(highFrame);
    }
    PolishThickProcessor lowLevelProcessor;
    lowLevelProcessor.Prepare({ kSampleRate, kFrameCount });
    lowLevelProcessor.SetParameters(snapshot);
    std::array<double, kFrameCount> lowLevelLeft{};
    std::array<double, kFrameCount> lowLevelRight{};
    MakeSine(lowLevelLeft, kAnalyzerFrequency, kLowLevelAmplitude);
    lowLevelRight = lowLevelLeft;
    DspProcessingFrame lowLevelFrame{
        { lowLevelLeft.data(), lowLevelRight.data(), lowLevelLeft.size() },
        {},
        plan,
        policy };
    for (auto block = 0; block < 40; ++block)
    {
        MakeSine(lowLevelLeft, kAnalyzerFrequency, kLowLevelAmplitude);
        lowLevelRight = lowLevelLeft;
        lowLevelProcessor.Process(lowLevelFrame);
    }
    auto highLevelHarmonicEnergy = 0.0;
    auto lowLevelHarmonicEnergy = 0.0;
    for (auto harmonic = 2.0; harmonic <= 20.0; harmonic += 1.0)
    {
        const auto highMagnitude = ToneMagnitude(
            highLeft,
            harmonic * kAnalyzerFrequency);
        const auto lowMagnitude = ToneMagnitude(
            lowLevelLeft,
            harmonic * kAnalyzerFrequency) / kLowLevelAmplitude;
        highLevelHarmonicEnergy += highMagnitude * highMagnitude;
        lowLevelHarmonicEnergy += lowMagnitude * lowMagnitude;
    }
    const auto highLevelHarmonics = std::sqrt(highLevelHarmonicEnergy);
    const auto normalizedLowHarmonics = std::sqrt(lowLevelHarmonicEnergy);
    succeeded &= Expect(
        normalizedLowHarmonics > highLevelHarmonics * 0.003 &&
            normalizedLowHarmonics < highLevelHarmonics * 0.02,
        "Thick did not match the documented low-level harmonic floor.");
    const auto lowLevelSecond = ToneMagnitude(
        lowLevelLeft,
        2.0 * kAnalyzerFrequency);
    const auto lowLevelThird = ToneMagnitude(
        lowLevelLeft,
        3.0 * kAnalyzerFrequency);
    succeeded &= Expect(
        lowLevelSecond > lowLevelThird * 10.0,
        "Thick retained the rich harmonic branch at a reduced input level.");
    return succeeded;
}

} // namespace consolidator::tests

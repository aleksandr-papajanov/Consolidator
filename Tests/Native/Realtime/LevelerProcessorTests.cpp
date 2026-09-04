#include "TestSupport.h"

#include <array>
#include <cmath>

#include "Dsp/DspProcessors.h"
#include "Dsp/DspRuntimeGraph.h"

namespace consolidator::tests
{

namespace
{

constexpr std::size_t kFrameCount = 100;
constexpr double kSampleRate = 1000.0;

void ProcessConstantLevel(
    consolidator::max::dsp::LevelerProcessor& leveler,
    double input,
    std::size_t blockCount,
    double& output)
{
    std::array<double, kFrameCount> left{};
    std::array<double, kFrameCount> right{};
    std::array<double, kFrameCount> detectorLeft{};
    std::array<double, kFrameCount> detectorRight{};
    auto plan = consolidator::max::dsp::DspProcessingPlan{};
    auto policy = consolidator::max::dsp::DspExecutionPolicy{};

    for (std::size_t block = 0; block < blockCount; ++block)
    {
        left.fill(input);
        right.fill(input);
        detectorLeft.fill(input);
        detectorRight.fill(input);
        auto main = consolidator::max::dsp::DspAudioBuffer{
            left.data(),
            right.data(),
            left.size()
        };
        auto frame = consolidator::max::dsp::DspProcessingFrame{
            main,
            { detectorLeft.data(), detectorRight.data() },
            plan,
            policy
        };
        leveler.Process(frame);
    }

    output = left.back();
}

}

bool RunLevelerProcessorTests()
{
    consolidator::max::dsp::LevelerProcessor leveler;
    leveler.Prepare({ kSampleRate, kFrameCount });

    consolidator::max::dsp::DspSnapshot snapshot{};
    snapshot.inputLeveler = 1;
    snapshot.inputTarget = -18.0F;
    leveler.SetParameters(snapshot);

    const auto inputAtMinusSixDb = std::pow(10.0, -6.0 / 20.0);
    auto output = 0.0;
    auto succeeded = true;

    consolidator::max::dsp::LevelerProcessor responsiveLeveler;
    responsiveLeveler.Prepare({ kSampleRate, kFrameCount });
    responsiveLeveler.SetParameters(snapshot);
    ProcessConstantLevel(
        responsiveLeveler,
        inputAtMinusSixDb,
        1,
        output);
    succeeded &= Expect(
        responsiveLeveler.FastInputLevelDb() > -6.0F &&
            responsiveLeveler.FastInputLevelDb() < 0.0F,
        "Leveler fast correction did not stay inside its six-decibel range.");
    succeeded &= Expect(
        std::abs(
            responsiveLeveler.EffectiveInputLevelDb() -
            responsiveLeveler.SlowInputLevelDb() -
            responsiveLeveler.FastInputLevelDb()) < 0.001F &&
            std::abs(responsiveLeveler.SlowInputLevelDb()) <
                std::abs(responsiveLeveler.EffectiveInputLevelDb()),
        "Leveler fast correction was not applied independently over the slow level.");
    ProcessConstantLevel(
        responsiveLeveler,
        inputAtMinusSixDb,
        39,
        output);

    succeeded &= Expect(
        responsiveLeveler.EffectiveInputLevelDb() < -9.0F,
        "Leveler fast correction did not move into the target region promptly.");

    consolidator::max::dsp::LevelerProcessor positiveCorrectionLeveler;
    positiveCorrectionLeveler.Prepare({ kSampleRate, kFrameCount });
    snapshot.inputTarget = -6.0F;
    positiveCorrectionLeveler.SetParameters(snapshot);
    const auto inputAtMinusEighteenDb = std::pow(10.0, -18.0 / 20.0);
    ProcessConstantLevel(
        positiveCorrectionLeveler,
        inputAtMinusEighteenDb,
        1,
        output);
    succeeded &= Expect(
        positiveCorrectionLeveler.FastInputLevelDb() > 0.0F &&
            positiveCorrectionLeveler.FastInputLevelDb() < 6.0F,
        "Leveler positive fast correction exceeded its six-decibel range.");

    snapshot.inputTarget = -18.0F;

    ProcessConstantLevel(leveler, inputAtMinusSixDb, 600, output);

    const auto targetAtMinusEighteenDb = std::pow(10.0, -18.0 / 20.0);
    succeeded &= Expect(
        std::abs(output - targetAtMinusEighteenDb) < 1.0e-4,
        "Leveler did not converge a sustained RMS input to its target level.");
    succeeded &= Expect(
        std::abs(leveler.FastInputLevelDb()) < 0.01F,
        "Leveler fast correction did not return to zero after slow settling.");

    const auto settledOutput = output;
    const auto settledSlowLevel = leveler.SlowInputLevelDb();
    ProcessConstantLevel(leveler, 1.0, 1, output);
    ProcessConstantLevel(leveler, inputAtMinusSixDb, 1, output);
    succeeded &= Expect(
        std::abs(leveler.FastInputLevelDb()) < 1.1F &&
            std::abs(output - settledOutput) < 0.016,
        "Leveler reacted excessively to a short level excursion.");
    succeeded &= Expect(
        std::abs(leveler.FastInputLevelDb()) >
            5.0F * std::abs(
                leveler.SlowInputLevelDb() - settledSlowLevel),
        "Leveler fast path did not move independently of the slow level.");

    ProcessConstantLevel(leveler, inputAtMinusSixDb, 600, output);
    const auto outputBeforeSilence = output;
    ProcessConstantLevel(leveler, 0.0, 600, output);
    ProcessConstantLevel(leveler, inputAtMinusSixDb, 1, output);
    succeeded &= Expect(
        std::abs(output - outputBeforeSilence) < 0.002,
        "Leveler raised its gain during sustained silence.");

    snapshot.inputTarget = -12.0F;
    leveler.SetParameters(snapshot);
    ProcessConstantLevel(leveler, inputAtMinusSixDb, 600, output);
    const auto targetAtMinusTwelveDb = std::pow(10.0, -12.0 / 20.0);
    succeeded &= Expect(
        std::abs(output - targetAtMinusTwelveDb) < 1.0e-4,
        "Leveler did not follow a changed target level.");

    snapshot.inputLevel = -6.0F;
    snapshot.inputLeveler = 0;
    leveler.SetParameters(snapshot);
    ProcessConstantLevel(leveler, inputAtMinusSixDb, 10, output);
    succeeded &= Expect(
        std::abs(output - targetAtMinusTwelveDb) < 1.0e-4,
        "Leveler did not retain its effective gain after being disabled.");

    snapshot.inputLevel = -3.0F;
    leveler.SetParameters(snapshot);
    ProcessConstantLevel(leveler, inputAtMinusSixDb, 1, output);
    succeeded &= Expect(
        std::abs(output - std::pow(10.0, -9.0 / 20.0)) < 1.0e-9,
        "A manual gain edit did not replace the disabled Leveler value.");

    consolidator::max::dsp::LevelerProcessor quietLeveler;
    quietLeveler.Prepare({ kSampleRate, kFrameCount });
    snapshot.inputLevel = 0.0F;
    snapshot.inputLeveler = 1;
    snapshot.inputTarget = -18.0F;
    quietLeveler.SetParameters(snapshot);
    const auto inputAtMinusNinetyDb = std::pow(10.0, -90.0 / 20.0);
    ProcessConstantLevel(
        quietLeveler,
        inputAtMinusNinetyDb,
        100,
        output);
    succeeded &= Expect(
        std::abs(quietLeveler.SlowInputLevelDb()) < 0.001F &&
            std::abs(quietLeveler.FastInputLevelDb()) < 0.001F,
        "Leveler gate boosted sustained signal below its threshold.");

    const auto inputAtMinusFiftyFourDb = std::pow(10.0, -54.0 / 20.0);
    ProcessConstantLevel(
        quietLeveler,
        inputAtMinusFiftyFourDb,
        1,
        output);
    const auto slowLevelAfterGateOpen = quietLeveler.SlowInputLevelDb();
    const auto inputAtMinusFiftyEightDb = std::pow(10.0, -58.0 / 20.0);
    ProcessConstantLevel(
        quietLeveler,
        inputAtMinusFiftyEightDb,
        1,
        output);
    succeeded &= Expect(
        quietLeveler.SlowInputLevelDb() > slowLevelAfterGateOpen,
        "Leveler slow gate did not remain open inside its hysteresis range.");

    const auto slowLevelBeforeGateClose = quietLeveler.SlowInputLevelDb();
    const auto inputAtMinusSixtyOneDb = std::pow(10.0, -61.0 / 20.0);
    ProcessConstantLevel(
        quietLeveler,
        inputAtMinusSixtyOneDb,
        100,
        output);
    succeeded &= Expect(
        std::abs(
            quietLeveler.SlowInputLevelDb() -
            slowLevelBeforeGateClose) < 0.001F &&
            std::abs(quietLeveler.FastInputLevelDb()) < 0.001F,
        "Leveler gate did not freeze slow learning and release fast correction.");

    consolidator::max::dsp::DspRuntimeGraph graph;
    graph.Prepare(kSampleRate, kFrameCount);
    snapshot.audible = 1;
    snapshot.inputGainActive = 1;
    snapshot.inputWidth = 100.0F;
    graph.SetParameters(snapshot);
    std::array<double, kFrameCount> graphLeft{};
    std::array<double, kFrameCount> graphRight{};
    for (auto block = 0; block < 600; ++block)
    {
        graphLeft.fill(inputAtMinusSixDb);
        graphRight.fill(inputAtMinusSixDb);
        graph.ProcessBlock(
            graphLeft.data(),
            graphRight.data(),
            graphLeft.size());
    }

    succeeded &= Expect(
        std::abs(graph.InputLevelTelemetry() + 12.0F) < 0.01F &&
            std::abs(graphLeft.back() - targetAtMinusEighteenDb) < 1.0e-4,
        "Input Leveler detector observed its own gain correction.");

    return succeeded;
}

} // namespace consolidator::tests

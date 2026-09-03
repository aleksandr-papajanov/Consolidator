#include "TestSupport.h"

#include <array>
#include <cmath>
#include <limits>

#include "Dsp/DspRuntimeGraph.h"
#include "Dsp/DspExecution.h"

namespace consolidator::tests
{

bool RunDspRuntimeGraphTests()
{
    consolidator::max::dsp::DspRuntimeGraph graph;
    graph.Prepare(48000.0, 4);

    auto context = graph.Context();
    auto succeeded = true;
    succeeded &= Expect(
        context.sampleRate == 48000.0 &&
            context.maximumFrameCount == 4,
        "DSP graph preparation did not retain its processing context.");

    consolidator::max::dsp::DspSnapshot identitySnapshot{};
    identitySnapshot.audible = 1;
        graph.SetParameters(identitySnapshot);

    consolidator::max::dsp::DspSnapshot snapshot{};
    snapshot.audible = 0;
    snapshot.instanceBypass = 1;
    snapshot.inputGainActive = 1;
    snapshot.inputLeveler = 1;
    snapshot.saturatorActive = 1;
    snapshot.saturatorSplit = 1;
    snapshot.compressorActive = 1;
    snapshot.compressorParallel = 1;
    snapshot.equalizerActive = 1;
    snapshot.polishBypass = 0;
    snapshot.outputGainActive = 1;
    snapshot.outputLimiter = 1;
    auto policy = consolidator::max::dsp::BuildExecutionPolicy(
        snapshot,
        graph.ProcessingPlan());
    succeeded &= Expect(
        !policy.audible && policy.instanceBypassed && policy.levelerEnabled &&
            policy.saturatorSplit && policy.compressorParallel &&
            policy.limiterEnabled && policy.widthProcessingRequired,
        "DSP execution policy did not project global runtime flags.");

    consolidator::max::dsp::DspProcessingPlan plan;
    plan.channelMode = consolidator::max::dsp::DspChannelMode::Left;
    succeeded &= Expect(
        plan.frequencyBands.Add(80.0, 250.0) &&
            plan.frequencyBands.Add(1000.0, 4000.0) &&
            !plan.frequencyBands.Add(
                std::numeric_limits<double>::quiet_NaN(),
                5000.0) &&
            !plan.frequencyBands.Add(4000.0, 1000.0),
        "DSP processing plan accepted an invalid frequency range.");
    graph.SetProcessingPlan(plan);
    succeeded &= Expect(
        graph.ProcessingPlan().channelMode ==
                consolidator::max::dsp::DspChannelMode::Left &&
            graph.ProcessingPlan().frequencyBands.count == 2,
        "DSP graph did not retain channel and frequency-band selection.");

    auto invalidPlan = plan;
    invalidPlan.frequencyBands.ranges[0].maximumHz = 30000.0;
    graph.SetProcessingPlan(invalidPlan);
    succeeded &= Expect(
        graph.ProcessingPlan().frequencyBands.ranges[0].maximumHz == 250.0,
        "DSP graph accepted a frequency band above the Nyquist limit.");

    std::array<double, 4> left{ 1.0, 2.0, 3.0, 4.0 };
    std::array<double, 4> right{ -1.0, -2.0, -3.0, -4.0 };
    const std::array<double, 4> sidechainLeft{ 5.0, 6.0, 7.0, 8.0 };
    const std::array<double, 4> sidechainRight{ -5.0, -6.0, -7.0, -8.0 };
    const auto originalLeft = left;
    const auto originalRight = right;

    graph.ProcessBlock(
        left.data(),
        right.data(),
        sidechainLeft.data(),
        sidechainRight.data(),
        left.size());

    succeeded &= Expect(
        left == originalLeft && right == originalRight,
        "Identity DSP foundation changed audio before an algorithm was added.");

    auto inputSnapshot = identitySnapshot;
    inputSnapshot.inputGainActive = 1;
    inputSnapshot.inputLevel = 6.0F;
    graph.SetParameters(inputSnapshot);
    graph.SetProcessingPlan({});
    graph.ProcessBlock(
        left.data(),
        right.data(),
        sidechainLeft.data(),
        sidechainRight.data(),
        left.size());
    const auto expectedGain = std::pow(10.0, 6.0 / 20.0);
    succeeded &= Expect(
        std::abs(left[0] - originalLeft[0] * expectedGain) < 1.0e-9 &&
            std::abs(right[0] - originalRight[0] * expectedGain) < 1.0e-9,
        "Input level did not apply its dB gain to both channels.");

    inputSnapshot.inputGainBypass = 1;
    graph.SetParameters(inputSnapshot);
    left = originalLeft;
    right = originalRight;
    graph.ProcessBlock(
        left.data(),
        right.data(),
        sidechainLeft.data(),
        sidechainRight.data(),
        left.size());
    succeeded &= Expect(
        left == originalLeft && right == originalRight,
        "Input gain bypass did not preserve the original signal.");

    graph.SetProcessingPlan(plan);
    graph.SetParameters(identitySnapshot);
    graph.ProcessBlock(
        left.data(),
        nullptr,
        sidechainLeft.data(),
        nullptr,
        left.size());
    succeeded &= Expect(
        left == originalLeft,
        "Left-only DSP processing rejected or changed its selected channel.");

    left[0] = 10.0;
    graph.ProcessBlock(
        left.data(),
        right.data(),
        sidechainLeft.data(),
        sidechainRight.data(),
        5);
    succeeded &= Expect(
        left[0] == 10.0,
        "DSP graph touched audio beyond its prepared frame capacity.");

    return succeeded;
}

} // namespace consolidator::tests

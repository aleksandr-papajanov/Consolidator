#include "TestSupport.h"

#include <array>
#include <cmath>
#include <limits>

#include "Dsp/DspExecution.h"
#include "Dsp/DspParallelStage.h"
#include "Dsp/DspRuntimeGraph.h"

namespace
{

class DoubleProcessor final : public consolidator::max::dsp::DspProcessor
{
public:
    void Prepare(const consolidator::max::dsp::DspProcessingContext&) noexcept override
    {
    }

    void Reset() noexcept override
    {
    }

    void SetParameters(const consolidator::max::dsp::DspSnapshot&) noexcept override
    {
    }

    void Process(consolidator::max::dsp::DspProcessingFrame& frame) noexcept override
    {
        for (std::size_t index = 0; index < frame.main.frameCount; ++index)
        {
            if (frame.main.left != nullptr)
            {
                frame.main.left[index] *= 2.0;
            }
            if (frame.main.right != nullptr)
            {
                frame.main.right[index] *= 2.0;
            }
        }
    }
};

}

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
    identitySnapshot.inputWidth = 100.0F;
    graph.SetParameters(identitySnapshot);

    consolidator::max::dsp::DspSnapshot snapshot{};
    snapshot.audible = 0;
    snapshot.instanceBypass = 1;
    snapshot.inputGainActive = 1;
    snapshot.inputLeveler = 1;
    snapshot.saturatorActive = 1;
    snapshot.saturatorDrive = 1.0F;
    snapshot.saturatorSplit = 1;
    snapshot.compressorActive = 1;
    snapshot.compressorCompression = 1.0F;
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
    const auto originalLeft = left;
    const auto originalRight = right;

    graph.ProcessBlock(
        left.data(),
        right.data(),
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
        left.size());
    const auto expectedGain = std::pow(10.0, 6.0 / 20.0);
    succeeded &= Expect(
        left[0] == originalLeft[0] &&
            left[3] > originalLeft[3] &&
            left[3] < originalLeft[3] * expectedGain,
        "Input level did not begin a smooth dB gain ramp.");

    for (auto block = 1; block < 120; ++block)
    {
        left = originalLeft;
        right = originalRight;
        graph.ProcessBlock(
            left.data(),
            right.data(),
            left.size());
    }

    left = originalLeft;
    right = originalRight;
    graph.ProcessBlock(
        left.data(),
        right.data(),
        left.size());
    succeeded &= Expect(
        std::abs(left[0] - originalLeft[0] * expectedGain) < 1.0e-9 &&
            std::abs(right[0] - originalRight[0] * expectedGain) < 1.0e-9,
        "Input level did not reach its exact target after the ramp.");

    inputSnapshot.inputGainBypass = 1;
    graph.SetParameters(inputSnapshot);
    left = originalLeft;
    right = originalRight;
    graph.ProcessBlock(
        left.data(),
        right.data(),
        left.size());
    succeeded &= Expect(
        left == originalLeft && right == originalRight,
        "Input gain bypass did not preserve the original signal.");

    consolidator::max::dsp::DetectorProcessor detector{ 0 };
    detector.Prepare(graph.Context());
    auto detectorMain = consolidator::max::dsp::DspAudioBuffer{
        left.data(),
        right.data(),
        left.size()
    };
    auto detectorPlan = consolidator::max::dsp::DspProcessingPlan{};
    auto detectorPolicy = consolidator::max::dsp::DspExecutionPolicy{};
    consolidator::max::dsp::DspProcessingFrame detectorFrame{
        detectorMain,
        {},
        detectorPlan,
        detectorPolicy
    };
    detector.Process(detectorFrame);
    succeeded &= Expect(
        detectorFrame.detectorSignal.left != left.data() &&
            detectorFrame.detectorSignal.right != right.data() &&
            detectorFrame.detectorSignal.left[0] == left[0] &&
            detectorFrame.detectorSignal.right[0] == right[0],
        "Detector processing did not copy the current main signal into its internal route.");

    consolidator::max::dsp::DspParallelStage parallel;
    parallel.Prepare(graph.Context());
    DoubleProcessor doubleProcessor;
    left.fill(1.0);
    right.fill(1.0);
    auto parallelMain = consolidator::max::dsp::DspAudioBuffer{
        left.data(),
        right.data(),
        left.size()
    };
    consolidator::max::dsp::DspProcessingFrame parallelFrame{
        parallelMain,
        {},
        detectorPlan,
        detectorPolicy
    };
    parallel.Process(parallelFrame, doubleProcessor);
    succeeded &= Expect(
        left[0] == 3.0 && right[0] == 3.0,
        "Parallel processing did not add the output-scaled wet branch to dry audio.");

    detectorPlan.channelMode = consolidator::max::dsp::DspChannelMode::Left;
    left.fill(1.0);
    right.fill(1.0);
    parallel.Process(parallelFrame, doubleProcessor);
    succeeded &= Expect(
        left[0] == 3.0 && right[0] == 1.0,
        "Parallel processing changed a channel excluded by the processing plan.");
    detectorPlan.channelMode = consolidator::max::dsp::DspChannelMode::Stereo;

    consolidator::max::dsp::DspRuntimeGraph topologyGraph;
    topologyGraph.Prepare(1000.0, 10);
    auto widthSnapshot = identitySnapshot;
    widthSnapshot.inputGainActive = 1;
    widthSnapshot.inputWidth = 0.0F;
    topologyGraph.SetParameters(widthSnapshot);
    std::array<double, 10> widthLeft{};
    std::array<double, 10> widthRight{};
    widthLeft.fill(1.0);
    widthRight.fill(-1.0);
    topologyGraph.ProcessBlock(
        widthLeft.data(),
        widthRight.data(),
        widthLeft.size());
    widthLeft.fill(1.0);
    widthRight.fill(-1.0);
    topologyGraph.ProcessBlock(
        widthLeft.data(),
        widthRight.data(),
        widthLeft.size());
    succeeded &= Expect(
        widthLeft[0] == 0.0 && widthRight[0] == 0.0,
        "Input width was not applied before the remaining device chain.");

    consolidator::max::dsp::DspRuntimeGraph compressorGraph;
    compressorGraph.Prepare(1000.0, 10);
    auto compressorSnapshot = identitySnapshot;
    compressorSnapshot.compressorActive = 1;
    compressorSnapshot.compressorParallel = 1;
    compressorSnapshot.compressorOutputDb = -6.0F;
    compressorSnapshot.outputLevel = 6.0F;
    compressorGraph.SetParameters(compressorSnapshot);
    std::array<double, 10> compressorLeft{};
    std::array<double, 10> compressorRight{};
    compressorLeft.fill(1.0);
    compressorRight.fill(1.0);
    compressorGraph.ProcessBlock(
        compressorLeft.data(),
        compressorRight.data(),
        compressorLeft.size());
    compressorLeft.fill(1.0);
    compressorRight.fill(1.0);
    compressorGraph.ProcessBlock(
        compressorLeft.data(),
        compressorRight.data(),
        compressorLeft.size());
    const auto compressorWetGain = std::pow(10.0, -6.0 / 20.0);
    const auto finalOutputGain = std::pow(10.0, 6.0 / 20.0);
    succeeded &= Expect(
        std::abs(
            compressorLeft[0] -
                ((1.0 + compressorWetGain) * finalOutputGain)) < 1.0e-9 &&
            std::abs(
                compressorRight[0] -
                    ((1.0 + compressorWetGain) * finalOutputGain)) < 1.0e-9,
        "Compressor output was not applied before its parallel sum and final output level.");

    consolidator::max::dsp::DspRuntimeGraph reactivationGraph;
    reactivationGraph.Prepare(1000.0, 10);
    auto activeSaturatorSnapshot = identitySnapshot;
    activeSaturatorSnapshot.saturatorActive = 1;
    activeSaturatorSnapshot.saturatorOutputDb = 6.0F;
    reactivationGraph.SetParameters(activeSaturatorSnapshot);
    std::array<double, 10> reactivationLeft{};
    std::array<double, 10> reactivationRight{};
    reactivationLeft.fill(1.0);
    reactivationRight.fill(1.0);
    reactivationGraph.ProcessBlock(
        reactivationLeft.data(),
        reactivationRight.data(),
        reactivationLeft.size());
    reactivationLeft.fill(1.0);
    reactivationRight.fill(1.0);
    reactivationGraph.ProcessBlock(
        reactivationLeft.data(),
        reactivationRight.data(),
        reactivationLeft.size());

    auto neutralSaturatorSnapshot = activeSaturatorSnapshot;
    neutralSaturatorSnapshot.saturatorOutputDb = 0.0F;
    reactivationGraph.SetParameters(neutralSaturatorSnapshot);
    auto reactivatedSaturatorSnapshot = neutralSaturatorSnapshot;
    reactivatedSaturatorSnapshot.saturatorDrive = 1.0F;
    reactivationGraph.SetParameters(reactivatedSaturatorSnapshot);
    reactivationLeft.fill(1.0);
    reactivationRight.fill(1.0);
    reactivationGraph.ProcessBlock(
        reactivationLeft.data(),
        reactivationRight.data(),
        reactivationLeft.size());
    succeeded &= Expect(
        reactivationLeft[0] == 1.0 && reactivationRight[0] == 1.0,
        "Saturator reactivation reused stale output-gain state.");

    consolidator::max::dsp::PolishProcessor polish;
    polish.Prepare(graph.Context());
    auto polishSnapshot = identitySnapshot;
    polishSnapshot.polishThick = 1.0F;
    polishSnapshot.polishAir = 1.0F;
    polish.SetParameters(polishSnapshot);
    left.fill(1.0);
    right.fill(-1.0);
    auto polishMain = consolidator::max::dsp::DspAudioBuffer{
        left.data(),
        right.data(),
        left.size()
    };
    consolidator::max::dsp::DspProcessingFrame polishFrame{
        polishMain,
        {},
        detectorPlan,
        detectorPolicy
    };
    polish.Process(polishFrame);
    succeeded &= Expect(
        left[0] == 1.0 && right[0] == -1.0,
        "The equal Thick/Air branch mix did not preserve unity for identity processing.");

    consolidator::max::dsp::DspRuntimeGraph channelGraph;
    channelGraph.Prepare(48000.0, 4);
    channelGraph.SetProcessingPlan(plan);
    channelGraph.SetParameters(identitySnapshot);
    left = originalLeft;
    right = originalRight;
    channelGraph.ProcessBlock(
        left.data(),
        nullptr,
        left.size());
    succeeded &= Expect(
        left == originalLeft,
        "Left-only DSP processing rejected or changed its selected channel.");

    left[0] = 10.0;
    channelGraph.ProcessBlock(
        left.data(),
        right.data(),
        5);
    succeeded &= Expect(
        left[0] == 10.0,
        "DSP graph touched audio beyond its prepared frame capacity.");

    return succeeded;
}

} // namespace consolidator::tests

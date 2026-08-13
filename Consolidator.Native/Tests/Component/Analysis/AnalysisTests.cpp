#include "Analysis/LatestSnapshot.h"
#include "Analysis/Spectrum/AudioWindowAccumulator.h"
#include "Analysis/Spectrum/SpectrumAnalyzer.h"
#include "Analysis/Spectrum/SpectrumMapper.h"
#include "Analysis/Spectrum/SpectrumStream.h"
#include "Analysis/FrequencyResponse/FrequencyResponseRequestBuilder.h"
#include "Analysis/LatestValue.h"
#include "Core/Analysis/AnalysisCurveInputBuilder.h"
#include "Dsp/DspChainBuilder.h"
#include "Dsp/Processors/DspChain.h"
#include "Dsp/Processors/Equalizer/Equalizer.h"
#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <array>
#include <vector>

using namespace consolidator;

namespace
{

struct TestSnapshot
{
    int value = 0;
    std::uint64_t revision = 0;
};

} // namespace

TEST_CASE("LatestSnapshot returns only the newest published revision")
{
    analysis::LatestSnapshot<TestSnapshot> mailbox;
    mailbox.Publish(TestSnapshot{1, 1});
    mailbox.Publish(TestSnapshot{3, 3});

    TestSnapshot output;
    EXPECT_TRUE(mailbox.TryReadNewerThan(output, 0));
    EXPECT_EQ(output.value, 3);
    EXPECT_EQ(output.revision, 3U);
    EXPECT_FALSE(mailbox.TryReadNewerThan(output, output.revision));
}

TEST_CASE("LatestValue allows repeated reads of one result")
{
    analysis::LatestValue<TestSnapshot> result;
    result.Publish(TestSnapshot{0, 7});

    TestSnapshot first;
    TestSnapshot second;
    EXPECT_TRUE(result.ReadLatest(first));
    EXPECT_TRUE(result.ReadLatest(second));
    EXPECT_EQ(first.revision, 7U);
    EXPECT_EQ(second.revision, 7U);

    TestSnapshot newer;
    EXPECT_TRUE(result.TryReadNewerThan(newer, 0));
    EXPECT_EQ(newer.revision, 7U);
    EXPECT_FALSE(result.TryReadNewerThan(newer, 7));
}

TEST_CASE("AudioWindowAccumulator publishes every complete window")
{
    analysis::AudioWindowAccumulator accumulator;
    accumulator.Prepare(48000.0);
    std::array<double, analysis::kFftSize * 2> left{};
    std::array<double, analysis::kFftSize * 2> right{};
    std::vector<analysis::AudioWindow> windows;

    accumulator.Push(
        left.data(), right.data(), left.size(),
        0,
        [&windows](const analysis::AudioWindow& window)
        {
            windows.push_back(window);
        });

    EXPECT_EQ(windows.size(), 2U);
    EXPECT_EQ(windows[0].revision, 1U);
    EXPECT_EQ(windows[1].revision, 2U);
    EXPECT_NEAR(windows[0].sampleRate, 48000.0, 0.0);
}

TEST_CASE("AudioWindowAccumulator resets a partial window between generations")
{
    analysis::AudioWindowAccumulator accumulator;
    accumulator.Prepare(48000.0);
    std::array<double, analysis::kFftSize> left{};
    std::array<double, analysis::kFftSize> right{};
    std::vector<analysis::AudioWindow> windows;

    accumulator.Push(
        left.data(), right.data(), analysis::kFftSize / 2,
        0,
        [&windows](const analysis::AudioWindow& window)
        {
            windows.push_back(window);
        });
    accumulator.Reset();
    accumulator.Push(
        left.data(), right.data(), analysis::kFftSize / 2,
        0,
        [&windows](const analysis::AudioWindow& window)
        {
            windows.push_back(window);
        });

    EXPECT_TRUE(windows.empty());

    accumulator.Push(
        left.data(), right.data(), analysis::kFftSize / 2,
        0,
        [&windows](const analysis::AudioWindow& window)
        {
            windows.push_back(window);
        });
    EXPECT_EQ(windows.size(), 1U);
    EXPECT_EQ(windows.front().generation, 0U);
}

TEST_CASE("SpectrumStream rejects windows from before an audio-thread reset")
{
    analysis::SpectrumStream stream;
    stream.Prepare(48000.0);
    std::array<double, analysis::kFftSize> left{};
    std::array<double, analysis::kFftSize> right{};
    analysis::AudioWindow window;

    stream.PushAudio(left.data(), right.data(), left.size());
    stream.Reset();
    EXPECT_FALSE(stream.TryConsumeInput(window));

    stream.PushAudio(left.data(), right.data(), left.size());
    EXPECT_TRUE(stream.TryConsumeInput(window));
    EXPECT_EQ(window.generation, 1U);
}

TEST_CASE("SpectrumAnalyzer normalizes a constant window at DC")
{
    analysis::SpectrumAnalyzer analyzer;
    analysis::AudioWindow input;
    input.leftSamples.fill(1.0F);
    input.rightSamples.fill(-1.0F);
    input.sampleRate = 48000.0;
    input.revision = 7;

    analysis::RawSpectrum raw;
    analyzer.Calculate(input, raw);

    EXPECT_NEAR(raw.magnitudes[0], 1.0, 0.01);
    EXPECT_EQ(raw.revision, 7U);
    EXPECT_NEAR(raw.sampleRate, 48000.0, 0.0);
}

TEST_CASE("SpectrumMapper creates a 256 point dB curve")
{
    analysis::RawSpectrum raw;
    raw.magnitudes.fill(1.0F);
    raw.sampleRate = 48000.0;
    raw.revision = 11;

    analysis::SpectrumSnapshot output;
    analysis::SpectrumMapper{}.Calculate(raw, output);

    EXPECT_EQ(output.magnitudeDb.size(), 256U);
    EXPECT_NEAR(output.magnitudeDb[0], 0.0, 0.001);
    EXPECT_NEAR(output.magnitudeDb[255], 0.0, 0.001);
    EXPECT_EQ(output.sourceRevision, 11U);
}

TEST_CASE("AnalysisCurveInputBuilder emits normalized EQ sections")
{
    core::StateStore stateStore;
    const auto input = core::AnalysisCurveInputBuilder{}.Build(
        stateStore.GetChain(), 48000.0, 5);
    const auto requests = analysis::FrequencyResponseRequestBuilder{}.Build(
        input, analysis::AnalysisView{core::InstanceId{0}, dsp::BankId::Bank0});
    const auto& request = requests.equalizer.combined;

    EXPECT_EQ(request.revision, 5U);
    EXPECT_NEAR(request.sampleRate, 48000.0, 0.0);
    // The worker calculates individual filters and the current/all-bank aggregates.
    EXPECT_EQ(request.stageCount, 8U);
    EXPECT_EQ(requests.equalizer.filters[0].stageCount, 1U);
    EXPECT_EQ(requests.equalizer.filters[1].stageCount, 2U);
    EXPECT_EQ(requests.equalizer.filters[2].stageCount, 1U);
    EXPECT_EQ(requests.equalizer.allBanksCombined.stageCount, 56U);
    EXPECT_NEAR(request.stages[0].b0, 1.0, 0.001);

    auto chain = dsp::DspChainBuilder{}.BuildStandardChain();
    chain->Prepare(48000.0, 2);
    const dsp::Equalizer* firstEqualizer = nullptr;
    for (std::size_t index = 0; index < chain->GetDeviceCount(); ++index)
    {
        firstEqualizer = dynamic_cast<const dsp::Equalizer*>(
            chain->GetDevice(index));
        if (firstEqualizer != nullptr)
        {
            break;
        }
    }

    EXPECT_TRUE(firstEqualizer != nullptr);
    if (firstEqualizer == nullptr || firstEqualizer->GetFilter(0) == nullptr)
    {
        return;
    }
    const auto& coefficients = firstEqualizer->GetFilter(0)
                                   ->GetRuntimeState()
                                   .coefficients;
    EXPECT_NEAR(request.stages[0].b0, coefficients.b0, 0.001);
    EXPECT_NEAR(request.stages[0].b1, coefficients.b1, 0.001);
    EXPECT_NEAR(request.stages[0].a1, coefficients.a1, 0.001);
}

TEST_CASE("FrequencyResponseRequestBuilder rejects an invalid bank")
{
    core::StateStore stateStore;
    const auto input = core::AnalysisCurveInputBuilder{}.Build(
        stateStore.GetChain(), 48000.0, 1);
    const auto requests = analysis::FrequencyResponseRequestBuilder{}.Build(
        input,
        analysis::AnalysisView{
            core::InstanceId{1},
            static_cast<dsp::BankId>(core::InstanceState::kBankCount)});

    EXPECT_EQ(requests.equalizer.combined.stageCount, 0U);
}

TEST_CASE("CurveState keeps the latest input available for repeated reads")
{
    analysis::CurveState state;
    analysis::CurveInput input;
    input.revision = 9;
    state.Publish(input);

    EXPECT_EQ(state.Read().revision, 9U);
    EXPECT_EQ(state.Read().revision, 9U);
}

TEST_CASE("AnalysisCurveInputBuilder follows the chain solo boundary")
{
    core::StateStore stateStore;
    const core::InstanceId instanceId{1};
    stateStore.SetInstanceId(instanceId);
    core::StateResponseEntries response;
    const auto status = stateStore.WriteState(
        test::Write(
            test::DevicePath(
                instanceId,
                dsp::DeviceId::Compressor,
                core::StateMarkerId::Solo),
            true),
        response);
    EXPECT_EQ(status, core::StateWriteStatus::Applied);

    const auto input = core::AnalysisCurveInputBuilder{}.Build(
        stateStore.GetChain(), 48000.0, 6);
    const auto requests = analysis::FrequencyResponseRequestBuilder{}.Build(
        input, analysis::AnalysisView{instanceId, dsp::BankId::Bank0});
    const auto& request = requests.equalizer.combined;
    EXPECT_EQ(request.stageCount, 0U);
}

TEST_CASE("AnalysisCurveInputBuilder emits detector responses with solo semantics")
{
    core::StateStore stateStore;
    const core::InstanceId instanceId{2};
    stateStore.SetInstanceId(instanceId);
    core::StateResponseEntries response;

    EXPECT_EQ(
        stateStore.WriteState(
            test::Write(test::DetectorFilterPath(
                instanceId, dsp::DeviceId::Compressor, 0,
                dsp::ParameterId::Gain), 6.0F),
            response),
        core::StateWriteStatus::Applied);
    EXPECT_EQ(
        stateStore.WriteState(
            test::Write(test::DetectorFilterPath(
                instanceId, dsp::DeviceId::Compressor, 0,
                dsp::ParameterId::Frequency), 750.0F),
            response),
        core::StateWriteStatus::Applied);
    EXPECT_EQ(
        stateStore.WriteState(
            test::Write(test::DetectorFilterPath(
                instanceId, dsp::DeviceId::Compressor, 0,
                dsp::ParameterId::Q), 2.0F),
            response),
        core::StateWriteStatus::Applied);
    EXPECT_EQ(
        stateStore.WriteState(
            test::Write(test::DetectorFilterPath(
                instanceId, dsp::DeviceId::Compressor, 1,
                core::StateMarkerId::Solo), true),
            response),
        core::StateWriteStatus::Applied);

    const auto input = core::AnalysisCurveInputBuilder{}.Build(
        stateStore.GetChain(), 48000.0, 12);
    const auto requests = analysis::FrequencyResponseRequestBuilder{}.Build(
        input, analysis::AnalysisView{instanceId, dsp::BankId::Bank0});

    EXPECT_NEAR(input.compressorDetector.filters[0].frequencyHz, 750.0F, 0.001F);
    EXPECT_NEAR(input.compressorDetector.filters[0].q, 2.0F, 0.001F);
    EXPECT_NEAR(input.compressorDetector.filters[0].gainDb, 6.0F, 0.001F);
    EXPECT_EQ(requests.compressorDetector.filters[0].stageCount, 1U);
    EXPECT_EQ(requests.compressorDetector.filters[1].stageCount, 1U);
    EXPECT_EQ(requests.compressorDetector.combined.stageCount, 1U);

    EXPECT_EQ(
        stateStore.WriteState(
            test::Write(test::DetectorFilterPath(
                instanceId, dsp::DeviceId::Compressor, 1,
                core::StateMarkerId::Bypass), true),
            response),
        core::StateWriteStatus::Applied);
    const auto bypassedInput = core::AnalysisCurveInputBuilder{}.Build(
        stateStore.GetChain(), 48000.0, 13);
    const auto bypassedRequests = analysis::FrequencyResponseRequestBuilder{}.Build(
        bypassedInput, analysis::AnalysisView{instanceId, dsp::BankId::Bank0});
    EXPECT_EQ(bypassedRequests.compressorDetector.filters[0].stageCount, 1U);
    EXPECT_EQ(bypassedRequests.compressorDetector.filters[1].stageCount, 1U);
    EXPECT_EQ(bypassedRequests.compressorDetector.combined.stageCount, 0U);
}

TEST_MAIN()

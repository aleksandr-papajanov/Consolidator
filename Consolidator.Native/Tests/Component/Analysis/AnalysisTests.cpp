#include "Analysis/LatestSnapshot.h"
#include "Analysis/Spectrum/AudioWindowAccumulator.h"
#include "Analysis/Spectrum/SpectrumAnalyzer.h"
#include "Analysis/Spectrum/SpectrumMapper.h"
#include "Core/Analysis/EqualizerResponseBuilder.h"
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

TEST_CASE("AudioWindowAccumulator publishes every complete window")
{
    analysis::AudioWindowAccumulator accumulator;
    accumulator.Prepare(48000.0);
    std::array<double, analysis::kFftSize * 2> left{};
    std::array<double, analysis::kFftSize * 2> right{};
    std::vector<analysis::AudioWindow> windows;

    accumulator.Push(
        left.data(), right.data(), left.size(),
        [&windows](const analysis::AudioWindow& window)
        {
            windows.push_back(window);
        });

    EXPECT_EQ(windows.size(), 2U);
    EXPECT_EQ(windows[0].revision, 1U);
    EXPECT_EQ(windows[1].revision, 2U);
    EXPECT_NEAR(windows[0].sampleRate, 48000.0, 0.0);
}

TEST_CASE("SpectrumAnalyzer normalizes a constant window at DC")
{
    analysis::SpectrumAnalyzer analyzer;
    analysis::AudioWindow input;
    input.samples.fill(1.0F);
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
    EXPECT_EQ(output.revision, 11U);
}

TEST_CASE("EqualizerResponseBuilder emits normalized EQ sections")
{
    core::StateStore stateStore;
    const auto request = core::EqualizerResponseBuilder{}.Build(
        stateStore, 48000.0, 5);

    EXPECT_EQ(request.revision, 5U);
    EXPECT_NEAR(request.sampleRate, 48000.0, 0.0);
    // Seven banks with seven filters each; each tilt filter expands to a
    // low-shelf/high-shelf pair, giving eight response stages per bank.
    EXPECT_EQ(request.stageCount, 56U);
    EXPECT_NEAR(request.stages[0].b0, 1.0, 0.001);

    auto chain = dsp::DspChainBuilder{}.BuildStandardChain();
    chain->Prepare(48000.0, 2);
    const dsp::Equalizer* firstEqualizer = nullptr;
    for (std::size_t index = 0; index < chain->GetDeviceCount(); ++index)
    {
        firstEqualizer = dynamic_cast<const dsp::Equalizer*>(
            chain->GetDevice(index));
        if (firstEqualizer != nullptr)
            break;
    }

    EXPECT_TRUE(firstEqualizer != nullptr);
    if (firstEqualizer == nullptr || firstEqualizer->GetFilter(0) == nullptr)
        return;
    const auto& coefficients = firstEqualizer->GetFilter(0)
        ->GetRuntimeState().coefficients;
    EXPECT_NEAR(request.stages[0].b0, coefficients.b0, 0.001);
    EXPECT_NEAR(request.stages[0].b1, coefficients.b1, 0.001);
    EXPECT_NEAR(request.stages[0].a1, coefficients.a1, 0.001);
}

TEST_CASE("EqualizerResponseBuilder follows the chain solo boundary")
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

    const auto request = core::EqualizerResponseBuilder{}.Build(
        stateStore, 48000.0, 6);
    EXPECT_EQ(request.stageCount, 0U);
}

TEST_MAIN()

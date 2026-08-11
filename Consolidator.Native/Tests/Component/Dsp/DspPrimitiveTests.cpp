#include "Dsp/Processors/Compressor/Compressor.h"
#include "Dsp/Processors/Compressor/RmsDetector.h"
#include "Dsp/Processors/Equalizer/Filters/BellFilter.h"
#include "Dsp/Processors/Equalizer/Filters/GainFilter.h"
#include "Dsp/Processors/Equalizer/Filters/TiltFilter.h"
#include "Dsp/Processors/Gain/Gain.h"
#include "Dsp/Processors/Saturator/DetectorEnvelopeFollower.h"
#include "Dsp/Processors/Saturator/Saturator.h"
#include "Dsp/Utilities/TimeCoefficient.h"
#include "Support/TestFramework.h"

#include <array>
#include <cmath>

using namespace consolidator;

namespace
{

void Stage(
    dsp::DspDevice& device,
    core::StatePath path,
    dsp::ParameterVariant value)
{
    EXPECT_TRUE(device.StageRuntimeUpdate(path, value));
    device.CommitRuntimeUpdates();
}

} // namespace

TEST_CASE("Time coefficient is bounded and increases with smoothing time")
{
    const auto fast = dsp::CalculateTimeCoefficient(1.0, 48000.0);
    const auto slow = dsp::CalculateTimeCoefficient(100.0, 48000.0);
    EXPECT_TRUE(fast >= 0.0 && fast < 1.0);
    EXPECT_TRUE(slow > fast && slow < 1.0);
    EXPECT_EQ(dsp::CalculateTimeCoefficient(-1.0, -1.0),
              dsp::CalculateTimeCoefficient(0.01, 1.0));
}

TEST_CASE("Gain stages runtime value and applies linear amplification")
{
    dsp::Gain gain{dsp::DeviceId::MainInputGain};
    Stage(gain, {dsp::DeviceId::MainInputGain, dsp::ParameterId::Gain}, 6.0f);
    EXPECT_FALSE(gain.IsNeutral());
    EXPECT_NEAR(gain.GetRuntimeState().linearGain, std::pow(10.0, 6.0 / 20.0), 1e-9);

    const std::array input{1.0, -0.5};
    std::array<double, 2> output{};
    gain.Process(input.data(), input.data() + 1,
                 output.data(), output.data() + 1, 1);
    EXPECT_NEAR(output[0], gain.GetRuntimeState().linearGain, 1e-9);
    EXPECT_NEAR(output[1], -0.5 * gain.GetRuntimeState().linearGain, 1e-9);
}

TEST_CASE("Filter parameter batch rebuilds coefficients and neutral state")
{
    dsp::BellFilter filter{dsp::FilterId::Filter1, 1000.0};
    filter.Prepare(48000.0, 2);
    EXPECT_TRUE(filter.IsNeutral());
    EXPECT_TRUE(filter.ApplyParameter(
        {dsp::DeviceId::Equalizer, dsp::ParameterId::Frequency}, 2500.0f, 0));
    EXPECT_TRUE(filter.ApplyParameter(
        {dsp::DeviceId::Equalizer, dsp::ParameterId::Q}, 2.0f, 0));
    EXPECT_TRUE(filter.ApplyParameter(
        {dsp::DeviceId::Equalizer, dsp::ParameterId::Gain}, 6.0f, 0));
    filter.CommitRuntimeUpdates();

    EXPECT_EQ(filter.GetRuntimeState().frequencyHz, 2500.0f);
    EXPECT_EQ(filter.GetRuntimeState().q, 2.0f);
    EXPECT_FALSE(filter.IsNeutral());
    EXPECT_FALSE(filter.GetRuntimeState().coefficients.b1 == 0.0);
}

TEST_CASE("Filter reset clears audio memory without changing parameters")
{
    dsp::GainFilter filter{dsp::FilterId::Filter1};
    filter.Prepare(48000.0, 2);
    EXPECT_TRUE(filter.ApplyParameter(
        {dsp::DeviceId::Equalizer, dsp::ParameterId::Gain}, 3.0f, 0));
    filter.CommitRuntimeUpdates();
    (void)filter.ProcessSample(1.0, 0);
    filter.Reset();

    EXPECT_EQ(filter.GetRuntimeState().gainDb, 3.0f);
    EXPECT_EQ(filter.GetRuntimeState().channelStates[0].z1, 0.0);
    EXPECT_EQ(filter.GetRuntimeState().channelStates[0].z2, 0.0);
}

TEST_CASE("RmsDetector fills a rolling window and reset clears the meter")
{
    dsp::RmsDetector detector;
    for (std::size_t index = 0; index < dsp::RmsDetector::kWindowSize; ++index)
    {
        EXPECT_NEAR(detector.ProcessSample(index % 2 == 0 ? 1.0 : -1.0), 1.0, 1e-12);
    }
    EXPECT_NEAR(detector.GetLevelLinear(), 1.0, 1e-6);
    detector.Reset();
    EXPECT_EQ(detector.GetLevelLinear(), 0.0f);
}

TEST_CASE("Saturator derives mix gain and detector state from one commit")
{
    dsp::Saturator saturator;
    saturator.Prepare(48000.0, 2);
    EXPECT_TRUE(saturator.StageRuntimeUpdate(
        {dsp::DeviceId::Saturator, dsp::ParameterId::Drive}, 2.0f));
    EXPECT_TRUE(saturator.StageRuntimeUpdate(
        {dsp::DeviceId::Saturator, dsp::ParameterId::Mix}, 0.25f));
    saturator.CommitRuntimeUpdates();

    EXPECT_EQ(saturator.GetRuntimeState().drive, 2.0f);
    EXPECT_EQ(saturator.GetRuntimeState().wetMix, 0.25);
    EXPECT_EQ(saturator.GetRuntimeState().dryMix, 0.75);
    EXPECT_FALSE(saturator.IsNeutral());
}

TEST_CASE("Compressor processes signal and publishes gain reduction")
{
    dsp::Compressor compressor;
    compressor.Prepare(48000.0, 2);
    Stage(compressor,
          {dsp::DeviceId::Compressor, dsp::ParameterId::Threshold}, -30.0f);

    std::array<double, 256> input;
    input.fill(1.0);
    std::array<double, 256> output{};
    compressor.Process(input.data(), input.data() + 128,
                       output.data(), output.data() + 128, 128);

    EXPECT_TRUE(compressor.GetGainReductionDb() < 0.0f);
    EXPECT_TRUE(std::abs(output.back()) < 1.0);
    compressor.Reset();
    EXPECT_EQ(compressor.GetGainReductionDb(), 0.0f);
}

TEST_MAIN()

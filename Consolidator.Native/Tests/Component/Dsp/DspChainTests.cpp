#include "Dsp/DspChainBuilder.h"
#include "Dsp/Processors/Compressor/Compressor.h"
#include "Dsp/Processors/DspChain.h"
#include "Dsp/Processors/Equalizer/Equalizer.h"
#include "Dsp/Processors/Equalizer/Filters/GainFilter.h"
#include "Dsp/Processors/Gain/Gain.h"
#include "Dsp/Processors/Saturator/Saturator.h"
#include "Dsp/Telemetry/MeterSmoother.h"
#include "Dsp/Telemetry/PeakMeter.h"
#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <array>
#include <cmath>
#include <cstdint>

using namespace consolidator;

namespace
{

core::ParameterUpdateBatch Batch(
    core::StatePath path,
    dsp::ParameterVariant value)
{
    core::ParameterUpdateBatch result;
    result.updates[0] = {std::move(path), value, 1};
    result.count = 1;
    result.revision = 1;
    return result;
}

} // namespace

TEST_CASE("Standard chain has stable stage and filter topology")
{
    auto chain = dsp::DspChainBuilder{}.BuildStandardChain();
    EXPECT_EQ(chain->GetDeviceCount(), 11U);
    EXPECT_EQ(chain->GetDevice(0)->GetDeviceId(), dsp::DeviceId::MainInputGain);
    EXPECT_EQ(chain->GetDevice(1)->GetDeviceId(), dsp::DeviceId::Saturator);
    EXPECT_EQ(chain->GetDevice(2)->GetDeviceId(), dsp::DeviceId::Compressor);
    EXPECT_EQ(chain->GetDevice(10)->GetDeviceId(), dsp::DeviceId::MainOutputGain);

    const auto* bank0 = dynamic_cast<const dsp::Equalizer*>(chain->GetDevice(3));
    EXPECT_TRUE(bank0 != nullptr);
    EXPECT_EQ(bank0->GetBankId(), dsp::BankId::Bank0);
    EXPECT_EQ(bank0->GetFilterCount(), 7U);
    EXPECT_TRUE(dynamic_cast<const dsp::GainFilter*>(bank0->GetFilter(0)) != nullptr);
}

TEST_CASE("Custom settings define bank identity and filter construction")
{
    core::settings::DspSettings settings;
    settings.banks[2].bankId = dsp::BankId::Bank5;
    settings.banks[2].bands[4].frequencyHz.defaultValue = 1500.0;
    auto chain = dsp::DspChainBuilder{}.BuildFromSettings(settings);

    const auto* bank2 = dynamic_cast<const dsp::Equalizer*>(chain->GetDevice(5));
    EXPECT_EQ(bank2->GetBankId(), dsp::BankId::Bank5);
    EXPECT_EQ(bank2->GetFilter(4)->GetRuntimeState().frequencyHz, 1500.0f);
}

TEST_CASE("Prepare propagates the host sample rate to sample-rate DSP state")
{
    auto chain = dsp::DspChainBuilder{}.BuildStandardChain();
    chain->Prepare(96000.0, 2);

    const auto* compressor =
        dynamic_cast<const dsp::Compressor*>(chain->GetDevice(2));
    const auto* bank0 =
        dynamic_cast<const dsp::Equalizer*>(chain->GetDevice(3));

    EXPECT_EQ(compressor->GetRuntimeState().sampleRate, 96000.0);
    EXPECT_EQ(bank0->GetFilter(0)->GetRuntimeState().sampleRate, 96000.0);
}

TEST_CASE("Runtime update routes to one bank and one filter")
{
    auto chain = dsp::DspChainBuilder{}.BuildStandardChain();
    chain->ApplyRuntimeUpdates(Batch(
        {dsp::DeviceId::Equalizer, dsp::ParameterId::Gain,
         dsp::RouteNodeId::Bank0, dsp::RouteNodeId::Filter3}, 6.0f));

    const auto* bank0 = dynamic_cast<const dsp::Equalizer*>(chain->GetDevice(3));
    const auto* bank1 = dynamic_cast<const dsp::Equalizer*>(chain->GetDevice(4));
    EXPECT_EQ(bank0->GetFilter(2)->GetRuntimeState().gainDb, 6.0f);
    EXPECT_EQ(bank0->GetFilter(3)->GetRuntimeState().gainDb, 0.0f);
    EXPECT_EQ(bank1->GetFilter(2)->GetRuntimeState().gainDb, 0.0f);
}

TEST_CASE("Runtime controls skip inactive stage and keep output deterministic")
{
    dsp::DspChain chain;
    chain.AddDevice(std::make_unique<dsp::Gain>(dsp::DeviceId::MainInputGain));
    chain.ApplyRuntimeUpdates(Batch(
        {dsp::DeviceId::MainInputGain, dsp::ParameterId::Gain}, 6.0f));
    core::RuntimeControlBatch controls;
    controls.updates[0] = {
        core::StatePath::Device(dsp::DeviceId::MainInputGain),
        core::RuntimeProperty::Active, false, 1};
    controls.count = 1;
    chain.ApplyRuntimeControlUpdates(controls);

    const std::array input{0.25, -0.5};
    std::array<double, 2> interim{};
    std::array<double, 2> output{};
    chain.Process(input.data(), input.data() + 1,
                  interim.data(), interim.data() + 1,
                  output.data(), output.data() + 1, 1);
    EXPECT_NEAR(output[0], input[0], 1e-9);
    EXPECT_NEAR(output[1], input[1], 1e-9);
}

TEST_CASE("Chain reset targets nested filter memory without changing its state")
{
    auto chain = dsp::DspChainBuilder{}.BuildStandardChain();
    const core::StatePath target{
        dsp::DeviceId::Equalizer, dsp::ParameterId::Gain,
        dsp::RouteNodeId::Bank0, dsp::RouteNodeId::Filter3};
    chain->ApplyRuntimeUpdates(Batch(target, 6.0f));
    auto* bank0 = dynamic_cast<dsp::Equalizer*>(chain->GetDevice(3));
    (void)bank0->GetFilter(2)->ProcessSample(1.0, 0);
    chain->Reset(target);

    EXPECT_EQ(bank0->GetFilter(2)->GetRuntimeState().gainDb, 6.0f);
    EXPECT_EQ(bank0->GetFilter(2)->GetRuntimeState().channelStates[0].z1, 0.0);
}

TEST_CASE("Telemetry meter points define the level storage")
{
    EXPECT_EQ(
        dsp::ToIndex(dsp::MeterPoint::Count),
        std::size_t{4});
    EXPECT_EQ(
        dsp::ToIndex(dsp::MeterPoint::CompressorOutput),
        std::size_t{2});

    dsp::TelemetrySnapshot snapshot;
    EXPECT_EQ(snapshot.levels.size(), dsp::ToIndex(dsp::MeterPoint::Count));
}

TEST_CASE("Meter smoother uses elapsed time instead of block count")
{
    dsp::MeterSmoother atFortyEightKilohertz{0.0f};
    atFortyEightKilohertz.SetSampleRate(48000.0);
    const auto first = atFortyEightKilohertz.Process(1.0f, 4800);

    dsp::MeterSmoother atTwentyFourKilohertz{0.0f};
    atTwentyFourKilohertz.SetSampleRate(24000.0);
    const auto second = atTwentyFourKilohertz.Process(1.0f, 2400);

    EXPECT_NEAR(first, second, 1e-6);
    EXPECT_NEAR(
        first,
        1.0 - std::exp(-0.1 / 0.150),
        1e-6);
}

TEST_CASE("Peak meter holds a transient while intermediate snapshots are dropped")
{
    dsp::PeakMeter meter;
    meter.SetSampleRate(48000.0);

    EXPECT_EQ(meter.Process(1.0f, 1), 1.0f);
    EXPECT_EQ(meter.Process(0.0f, 2400), 1.0f);
    EXPECT_TRUE(meter.Process(0.0f, 4800) < 1.0f);
    EXPECT_TRUE(meter.Process(0.0f, 4800) > 0.0f);
}

TEST_CASE("Chain level smoothing operates on linear RMS before dB conversion")
{
    auto chain = dsp::DspChainBuilder{}.BuildStandardChain();
    chain->Prepare(48000.0, 2);

    std::array<double, 4800> input{};
    input.fill(1.0);
    std::array<double, 4800> interim{};
    std::array<double, 4800> output{};
    chain->Process(
        input.data(), input.data(),
        interim.data(), interim.data(),
        output.data(), output.data(), input.size());
    const auto telemetry = chain->FinishTelemetryBlock(input.size());

    const auto expectedSmoothedDb = 20.0 * std::log10(
        1.0 - std::exp(-0.1 / 0.150));
    const auto& level = telemetry.levels[
        dsp::ToIndex(dsp::MeterPoint::InputGainOutput)];
    EXPECT_NEAR(level.rmsDb, 0.0, 1e-6);
    EXPECT_NEAR(level.peakDb, 0.0, 1e-6);
    EXPECT_NEAR(level.smoothedDb, expectedSmoothedDb, 1e-5);
}

TEST_CASE("Disabled chain telemetry skips block accumulation")
{
    dsp::DspChain chain;
    chain.AddDevice(std::make_unique<dsp::Gain>(dsp::DeviceId::MainInputGain));
    chain.SetTelemetryEnabled(false);

    std::array<double, 64> input{};
    input.fill(1.0);
    std::array<double, 64> interim{};
    std::array<double, 64> output{};
    chain.Process(
        input.data(), input.data(),
        interim.data(), interim.data(),
        output.data(), output.data(), input.size());

    const auto telemetry = chain.FinishTelemetryBlock(input.size());
    EXPECT_EQ(telemetry.revision, std::uint64_t{0});
    EXPECT_EQ(telemetry.levels[
                  dsp::ToIndex(dsp::MeterPoint::InputGainOutput)].rmsDb,
              -240.0f);
}

TEST_CASE("Compressor telemetry reports positive reduction from linear attenuation")
{
    dsp::Compressor compressor;
    compressor.Prepare(48000.0, 2);
    compressor.StageRuntimeUpdate(
        {dsp::DeviceId::Compressor, dsp::ParameterId::Threshold}, -30.0f);
    compressor.CommitRuntimeUpdates();

    std::array<double, 256> input{};
    input.fill(1.0);
    std::array<double, 256> output{};
    compressor.Process(
        input.data(), input.data() + 128,
        output.data(), output.data() + 128, 128);

    const auto telemetry = compressor.GetBlockTelemetry();
    EXPECT_TRUE(telemetry.gainReductionRmsDb > 0.0f);
    EXPECT_TRUE(telemetry.gainReductionPeakDb >= telemetry.gainReductionRmsDb);
}

TEST_CASE("Saturator distortion excludes output gain and wet dry mix")
{
    const auto process = [](float outputDb, float mix)
    {
        dsp::Saturator saturator;
        saturator.Prepare(48000.0, 2);
        saturator.StageRuntimeUpdate(
            {dsp::DeviceId::Saturator, dsp::ParameterId::Drive}, 2.0f);
        saturator.StageRuntimeUpdate(
            {dsp::DeviceId::Saturator, dsp::ParameterId::Gain}, outputDb);
        saturator.StageRuntimeUpdate(
            {dsp::DeviceId::Saturator, dsp::ParameterId::Mix}, mix);
        saturator.CommitRuntimeUpdates();

        std::array<double, 128> input{};
        input.fill(0.25);
        std::array<double, 128> output{};
        saturator.Process(
            input.data(), input.data(),
            output.data(), output.data(), input.size());
        return saturator.GetBlockTelemetry().distortionPercent;
    };

    const auto reference = process(0.0f, 1.0f);
    const auto changedOutputStage = process(12.0f, 0.25f);
    EXPECT_NEAR(reference, changedOutputStage, 1e-5);
}

TEST_CASE("Saturator distortion ignores inactive channels")
{
    dsp::Saturator mono;
    mono.Prepare(48000.0, 1);
    mono.StageRuntimeUpdate(
        {dsp::DeviceId::Saturator, dsp::ParameterId::Drive}, 2.0f);
    mono.CommitRuntimeUpdates();

    std::array<double, 128> left{};
    left.fill(0.25);
    std::array<double, 128> silentRight{};
    std::array<double, 128> outputLeft{};
    std::array<double, 128> outputRight{};
    mono.Process(
        left.data(), silentRight.data(),
        outputLeft.data(), outputRight.data(), left.size());

    const auto monoDistortion = mono.GetBlockTelemetry().distortionPercent;

    dsp::Saturator stereo;
    stereo.Prepare(48000.0, 2);
    stereo.StageRuntimeUpdate(
        {dsp::DeviceId::Saturator, dsp::ParameterId::Drive}, 2.0f);
    stereo.CommitRuntimeUpdates();
    stereo.Process(
        left.data(), left.data(),
        outputLeft.data(), outputRight.data(), left.size());

    EXPECT_NEAR(
        monoDistortion,
        stereo.GetBlockTelemetry().distortionPercent,
        1e-5);
}

TEST_MAIN()

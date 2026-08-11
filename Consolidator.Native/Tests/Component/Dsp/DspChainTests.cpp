#include "Dsp/DspChainBuilder.h"
#include "Dsp/Processors/Compressor/Compressor.h"
#include "Dsp/Processors/DspChain.h"
#include "Dsp/Processors/Equalizer/Equalizer.h"
#include "Dsp/Processors/Equalizer/Filters/GainFilter.h"
#include "Dsp/Processors/Gain/Gain.h"
#include "Dsp/Processors/Saturator/Saturator.h"
#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <array>

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

TEST_MAIN()

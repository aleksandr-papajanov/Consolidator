#include "Support/ProtocolDriver.h"
#include "Dsp/Processors/DspChain.h"
#include "Dsp/Processors/Gain/Gain.h"

#include <algorithm>
#include <cmath>

using namespace consolidator;

TEST_CASE("Instance preserves reference audio and applies state before main processing")
{
    test::ProtocolDriver driver{1};
    driver.MainInput().fill(1.0);
    for (std::size_t index = 0; index < driver.ReferenceInput().size(); ++index)
    {
        driver.ReferenceInput()[index] = static_cast<double>(index) / 10.0;
    }
    const auto id = driver.At(0).GetInstanceId();
    const auto response = driver.Write(0, 2000, test::Entries({
        test::Write(test::DevicePath(id, dsp::DeviceId::MainInputGain,
                                     dsp::ParameterId::Gain), 6.0f),
        test::Write(test::DevicePath(id, dsp::DeviceId::Saturator,
                                     dsp::ParameterId::Bypass), true),
        test::Write(test::DevicePath(id, dsp::DeviceId::Compressor,
                                     dsp::ParameterId::Bypass), true),
        test::Write(test::DevicePath(id, dsp::DeviceId::Equalizer,
                                     dsp::ParameterId::Bypass), true)}));
    const auto& gainResponse = test::FindEntry(response, test::DevicePath(
        id, dsp::DeviceId::MainInputGain, dsp::ParameterId::Gain));
    EXPECT_TRUE(gainResponse.status.has_value());
    EXPECT_EQ(static_cast<int>(*gainResponse.status),
              static_cast<int>(core::StateWriteStatus::Applied));
    EXPECT_EQ(driver.At(0).GetStateStore().GetChain().inputGain.gainDb.value, 6.0f);
    driver.ProcessAll();

    const auto* gain = dynamic_cast<const dsp::Gain*>(
        driver.At(0).GetDspChain().GetDevice(0));
    EXPECT_TRUE(gain != nullptr);
    EXPECT_EQ(gain->GetRuntimeState().gainDb, 6.0f);
    EXPECT_TRUE(driver.At(0).IsOutputEnabled());
    EXPECT_NEAR(driver.MainOutput()[0], std::pow(10.0, 6.0 / 20.0), 1e-6);
    EXPECT_TRUE(std::equal(driver.ReferenceInput().begin(),
                           driver.ReferenceInput().end(),
                           driver.ReferenceOutput().begin()));
}

TEST_CASE("Instance mute resolves to an output gate without changing state")
{
    test::ProtocolDriver driver{1};
    driver.MainInput().fill(0.5);
    const auto id = driver.At(0).GetInstanceId();
    (void)driver.Write(0, 2100, test::Entries({
        test::Write(core::StatePath::InstanceMute(id), true)}));
    driver.ProcessAll();

    EXPECT_FALSE(driver.At(0).IsOutputEnabled());
    EXPECT_TRUE(std::all_of(driver.MainOutput().begin(), driver.MainOutput().end(),
                            [](double sample) { return sample == 0.0; }));
    const auto response = driver.Read(0, 2101, core::StatePath::InstanceMute(id));
    EXPECT_TRUE(std::get<bool>(test::FindEntry(
        response, core::StatePath::InstanceMute(id)).value));
}

TEST_CASE("Output solo enables direct group members and gates unrelated instances")
{
    test::ProtocolDriver driver{3};
    const auto first = driver.At(0).GetInstanceId();
    const auto second = driver.At(1).GetInstanceId();
    const auto third = driver.At(2).GetInstanceId();
    (void)driver.Write(0, 2200, test::Entries({test::Write(
        core::StatePath::BankGroup(first, dsp::BankId::Bank0), core::GroupId{9})}));
    (void)driver.Write(1, 2201, test::Entries({test::Write(
        core::StatePath::BankGroup(second, dsp::BankId::Bank0), core::GroupId{9})}));
    (void)driver.Write(0, 2202, test::Entries({test::Write(
        core::StatePath::InstanceSolo(first), true)}));
    driver.ProcessAll();

    EXPECT_TRUE(driver.At(0).IsOutputEnabled());
    EXPECT_TRUE(driver.At(1).IsOutputEnabled());
    EXPECT_FALSE(driver.At(2).IsOutputEnabled());
    EXPECT_FALSE(third == first || third == second);
}

TEST_CASE("Latest-value delivery exposes the final parameter at block start")
{
    test::ProtocolDriver driver{1};
    const auto id = driver.At(0).GetInstanceId();
    const auto gain = test::DevicePath(
        id, dsp::DeviceId::MainInputGain, dsp::ParameterId::Gain);
    (void)driver.Write(0, 2299, test::Entries({
        test::Write(test::DevicePath(id, dsp::DeviceId::Saturator,
                                     dsp::ParameterId::Bypass), true),
        test::Write(test::DevicePath(id, dsp::DeviceId::Compressor,
                                     dsp::ParameterId::Bypass), true),
        test::Write(test::DevicePath(id, dsp::DeviceId::Equalizer,
                                     dsp::ParameterId::Bypass), true)}));
    driver.At(0).EnqueueCommand(core::WriteStateCommand{
        .requestId = 2300,
        .entries = test::Entries({test::Write(gain, 3.0f)})});
    driver.At(0).EnqueueCommand(core::WriteStateCommand{
        .requestId = 2301,
        .entries = test::Entries({test::Write(gain, 9.0f)})});

    const auto final = driver.Read(0, 2302, gain);
    EXPECT_EQ(std::get<float>(test::FindEntry(final, gain).value), 9.0f);
    EXPECT_EQ(driver.At(0).GetStateStore().GetChain().inputGain.gainDb.value, 9.0f);
    driver.MainInput().fill(1.0);
    driver.ProcessAll();
    EXPECT_NEAR(driver.MainOutput()[0], std::pow(10.0, 9.0 / 20.0), 1e-6);
}

TEST_MAIN()

#include "Support/ProtocolDriver.h"
#include "Dsp/Processors/Compressor/Compressor.h"
#include "Dsp/Processors/DspChain.h"
#include "Dsp/Processors/Saturator/Saturator.h"

#include <atomic>
#include <variant>

using namespace consolidator;

TEST_CASE("State protocol writes and reads top-level parameters and markers")
{
    test::ProtocolDriver driver{1};
    const auto id = driver.At(0).GetInstanceId();
    const std::array writes{
        test::Write(test::DevicePath(id, dsp::DeviceId::MainInputGain,
                                     dsp::ParameterId::Gain), 6.0f),
        test::Write(test::DevicePath(id, dsp::DeviceId::Saturator,
                                     core::StateMarkerId::Bypass), true),
        test::Write(test::DevicePath(id, dsp::DeviceId::Compressor,
                                     core::StateMarkerId::Bypass), true),
        test::Write(test::DevicePath(id, dsp::DeviceId::MainOutputGain,
                                     dsp::ParameterId::Gain), -3.0f)};
    core::RequestId request = 1000;

    for (const auto& entry : writes)
    {
        const auto writeResponse = driver.Write(0, request++, test::Entries({entry}));
        const auto& written = test::FindEntry(writeResponse, entry.path);
        EXPECT_TRUE(written.status.has_value());
        EXPECT_EQ(static_cast<int>(*written.status),
                  static_cast<int>(core::StateWriteStatus::Applied));
        const auto readResponse = driver.Read(0, request++, entry.path);
        EXPECT_EQ(test::FindEntry(readResponse, entry.path).value, entry.value);
    }
}

TEST_CASE("Identical request ids stay isolated between instance response queues")
{
    test::ProtocolDriver driver{2};
    constexpr core::RequestId requestId = 1050;
    const auto firstPath = core::StatePath::InstanceMute(
        driver.At(0).GetInstanceId());
    const auto secondPath = core::StatePath::InstanceMute(
        driver.At(1).GetInstanceId());

    driver.EnqueueRead(0, requestId, firstPath);
    driver.EnqueueRead(1, requestId, secondPath);

    const auto firstResponse = driver.AwaitRead(0, requestId);
    const auto secondResponse = driver.AwaitRead(1, requestId);

    EXPECT_EQ(firstResponse.instanceId, driver.At(0).GetInstanceId());
    EXPECT_EQ(secondResponse.instanceId, driver.At(1).GetInstanceId());
    EXPECT_EQ(test::FindEntry(firstResponse, firstPath).path, firstPath);
    EXPECT_EQ(test::FindEntry(secondResponse, secondPath).path, secondPath);
}

TEST_CASE("Response notifier signals after an instance response is queued")
{
    std::atomic<std::size_t> notifications{0};
    test::ProtocolDriver driver{1, [&notifications]
    {
        notifications.fetch_add(1, std::memory_order_relaxed);
    }};

    (void)driver.Read(
        0,
        1051,
        core::StatePath::InstanceMute(driver.At(0).GetInstanceId()));

    EXPECT_TRUE(notifications.load(std::memory_order_relaxed) > 0);
}

TEST_CASE("Response notifier is immutable after instance initialization")
{
    test::ProtocolDriver driver{1};
    EXPECT_FALSE(driver.At(0).SetResponseNotifier([] {}));
}

TEST_CASE("Destroying an instance with a pending response notifier is safe")
{
    std::atomic<std::size_t> notifications{0};
    {
        test::ProtocolDriver driver{1, [&notifications]
        {
            notifications.fetch_add(1, std::memory_order_relaxed);
        }};
        driver.EnqueueRead(
            0,
            1054,
            core::StatePath::InstanceMute(driver.At(0).GetInstanceId()));
    }
    EXPECT_TRUE(notifications.load(std::memory_order_relaxed) <= 1);
}

TEST_CASE("Protocol driver preserves unrelated in-flight responses")
{
    test::ProtocolDriver driver{1};
    const auto path = core::StatePath::InstanceMute(
        driver.At(0).GetInstanceId());

    driver.EnqueueRead(0, 1052, path);
    driver.EnqueueRead(0, 1053, path);

    (void)driver.AwaitRead(0, 1053);
    const auto firstResponse = driver.AwaitRead(0, 1052);
    EXPECT_EQ(firstResponse.requestId, 1052U);
}

TEST_CASE("Late DSP parameters are registered and applied at block boundary")
{
    test::ProtocolDriver driver{1};
    const auto id = driver.At(0).GetInstanceId();
    const auto compressorRatio = test::DevicePath(
        id, dsp::DeviceId::Compressor, dsp::ParameterId::Ratio);
    const auto saturatorDrive = test::DevicePath(
        id, dsp::DeviceId::Saturator, dsp::ParameterId::Drive);
    const auto detectorFrequency = test::DetectorFilterPath(
        id, dsp::DeviceId::Saturator, 0, dsp::ParameterId::Frequency);
    const auto compressorDetectorFrequency = test::DetectorFilterPath(
        id, dsp::DeviceId::Compressor, 0, dsp::ParameterId::Frequency);

    (void)driver.Write(0, 1055, test::Entries({test::Write(compressorRatio, 8.0f)}));
    (void)driver.Write(0, 1056, test::Entries({test::Write(saturatorDrive, 2.0f)}));
    (void)driver.Write(0, 1057, test::Entries({test::Write(detectorFrequency, 250.0f)}));
    (void)driver.Write(0, 1058, test::Entries({test::Write(
        compressorDetectorFrequency, 300.0f)}));
    driver.ProcessAll();

    auto& chain = driver.At(0).GetDspChain();
    const auto* compressor = dynamic_cast<const dsp::Compressor*>(chain.GetDevice(2));
    const auto* saturator = dynamic_cast<const dsp::Saturator*>(chain.GetDevice(1));
    const auto* compressorDetector = compressor == nullptr
        ? nullptr
        : &compressor->GetDetectorEqualizer();
    EXPECT_TRUE(compressor != nullptr);
    EXPECT_TRUE(saturator != nullptr);
    EXPECT_EQ(compressor->GetRuntimeState().ratio, 8.0f);
    EXPECT_EQ(saturator->GetRuntimeState().drive, 2.0f);
    EXPECT_EQ(saturator->GetDetector(0).GetEqualizer().GetFilter(0)
                  ->GetRuntimeState().frequencyHz, 250.0f);
    EXPECT_TRUE(compressorDetector != nullptr);
    EXPECT_EQ(compressorDetector->GetFilter(0)->GetRuntimeState().frequencyHz, 300.0f);
}

TEST_CASE("Grouped EQ write fans out to direct member banks")
{
    test::ProtocolDriver driver{2};
    const auto firstId = driver.At(0).GetInstanceId();
    const auto secondId = driver.At(1).GetInstanceId();
    constexpr core::RequestId base = 1100;
    (void)driver.Write(0, base, test::Entries({test::Write(
        core::StatePath::BankGroup(firstId, dsp::BankId::Bank0), core::GroupId{42})}));
    (void)driver.Write(1, base + 1, test::Entries({test::Write(
        core::StatePath::BankGroup(secondId, dsp::BankId::Bank2), core::GroupId{42})}));
    EXPECT_EQ(core::InstanceCoordinator::Get().GetRegistry()
                  .FindGroupMembers(core::GroupId{42}).size(), 2U);
    const core::GroupGraph graph{
        core::InstanceCoordinator::Get().GetRegistry()};
    const core::StateRouter router{
        core::InstanceCoordinator::Get().GetRegistry(), graph};
    EXPECT_EQ(router.ResolveWriteTargets(firstId, test::FilterPath(
        firstId, dsp::BankId::Bank0, 2, dsp::ParameterId::Gain)).size(), 2U);

    const auto sourcePath = test::FilterPath(
        firstId, dsp::BankId::Bank0, 2, dsp::ParameterId::Gain);
    const auto targetPath = test::FilterPath(
        secondId, dsp::BankId::Bank2, 2, dsp::ParameterId::Gain);
    const auto response = driver.Write(0, base + 2, test::Entries({
        test::Write(sourcePath, 7.0f)}));

    const auto& sourceEntry = test::FindEntry(response, sourcePath);
    EXPECT_TRUE(sourceEntry.status.has_value());
    EXPECT_EQ(static_cast<int>(*sourceEntry.status),
              static_cast<int>(core::StateWriteStatus::Applied));
    EXPECT_EQ(std::get<float>(sourceEntry.value), 7.0f);
    EXPECT_EQ(std::get<float>(test::FindEntry(
        driver.Read(1, base + 3, targetPath), targetPath).value), 7.0f);
}

TEST_CASE("Instance-owned writes use direct group but refresh connected constraints")
{
    test::ProtocolDriver driver{3};
    const auto first = driver.At(0).GetInstanceId();
    const auto second = driver.At(1).GetInstanceId();
    const auto third = driver.At(2).GetInstanceId();
    core::RequestId request = 1200;
    const auto group = [&](std::size_t instance, core::InstanceId id,
                           dsp::BankId bank, core::GroupId groupId)
    {
        (void)driver.Write(instance, request++, test::Entries({test::Write(
            core::StatePath::BankGroup(id, bank), groupId)}));
    };
    group(0, first, dsp::BankId::Bank0, core::GroupId{1});
    group(1, second, dsp::BankId::Bank0, core::GroupId{1});
    group(1, second, dsp::BankId::Bank1, core::GroupId{2});
    group(2, third, dsp::BankId::Bank0, core::GroupId{2});
    EXPECT_EQ(core::InstanceCoordinator::Get().GetRegistry()
                  .FindGroupMembers(core::GroupId{1}).size(), 2U);
    EXPECT_EQ(core::InstanceCoordinator::Get().GetRegistry()
                  .FindGroupMembers(core::GroupId{2}).size(), 2U);

    const auto firstGain = test::DevicePath(
        first, dsp::DeviceId::MainInputGain, dsp::ParameterId::Gain);
    const auto secondGain = test::DevicePath(
        second, dsp::DeviceId::MainInputGain, dsp::ParameterId::Gain);
    const auto thirdGain = test::DevicePath(
        third, dsp::DeviceId::MainInputGain, dsp::ParameterId::Gain);
    const auto response = driver.Write(0, request++, test::Entries({
        test::Write(firstGain, 2.0f)}));

    const auto& sourceEntry = test::FindEntry(response, firstGain);
    EXPECT_TRUE(sourceEntry.status.has_value());
    EXPECT_EQ(static_cast<int>(*sourceEntry.status),
              static_cast<int>(core::StateWriteStatus::Applied));
    EXPECT_EQ(std::get<float>(sourceEntry.value), 2.0f);
    EXPECT_EQ(std::get<float>(test::FindEntry(
        driver.Read(1, request++, secondGain), secondGain).value), 2.0f);
    EXPECT_EQ(std::get<float>(test::FindEntry(
        driver.Read(2, request++, thirdGain), thirdGain).value), 0.0f);
    EXPECT_TRUE(test::FindEntry(response, firstGain).minimum.has_value());
}

TEST_CASE("Rejected entry does not roll back accepted entries in the same batch")
{
    test::ProtocolDriver driver{1};
    const auto id = driver.At(0).GetInstanceId();
    auto invalid = core::StatePath::Instance(id);
    invalid.field = core::StateField::DspParameter;
    const auto gain = test::DevicePath(
        id, dsp::DeviceId::MainOutputGain, dsp::ParameterId::Gain);
    const auto response = driver.Write(0, 1300, test::Entries({
        test::Write(gain, -6.0f), test::Write(invalid, true)}));

    EXPECT_EQ(response.entries.size, 2U);
    EXPECT_EQ(test::FindEntry(response, gain).status, core::StateWriteStatus::Applied);
    EXPECT_EQ(std::get<float>(test::FindEntry(
        driver.Read(0, 1301, gain), gain).value), -6.0f);
}

TEST_MAIN()

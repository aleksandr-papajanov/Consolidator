#include "Support/ProtocolDriver.h"

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
                                     dsp::ParameterId::Bypass), true),
        test::Write(test::DevicePath(id, dsp::DeviceId::Compressor,
                                     dsp::ParameterId::Bypass), true),
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

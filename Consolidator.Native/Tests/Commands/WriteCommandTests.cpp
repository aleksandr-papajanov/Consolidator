#include "Support/CommandFixture.h"
#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <string>
#include <variant>

using namespace consolidator;

namespace
{

const core::StateWriteResult& Write(
    test::CommandFixture& fixture,
    core::RequestId requestId,
    core::StateRequestEntries entries)
{
    static core::CommandResult result;
    result = fixture.commandRouter.HandleCommand(core::WriteStateCommand{
        .requestId = requestId,
        .instanceId = fixture.instance.GetInstanceId(),
        .entries = std::move(entries)});
    EXPECT_TRUE(std::holds_alternative<core::StateWriteResult>(result));
    return std::get<core::StateWriteResult>(result);
}

} // namespace

TEST_CASE("Write command applies a DSP parameter and returns authoritative state")
{
    test::CommandFixture fixture;
    const auto id = fixture.instance.GetInstanceId();
    const auto path = test::DevicePath(
        id, dsp::DeviceId::MainInputGain, dsp::ParameterId::Gain);
    const auto& result = Write(fixture, 200, test::Entries({
        test::Write(path, 6.0f)}));

    EXPECT_EQ(result.response.requestId, 200U);
    EXPECT_EQ(result.response.entries.size, 1U);
    EXPECT_EQ(result.response.entries.entries[0].status,
              core::StateWriteStatus::Applied);
    EXPECT_EQ(std::get<float>(result.response.entries.entries[0].value), 6.0f);
    EXPECT_TRUE(result.response.entries.entries[0].minimum.has_value());
}

TEST_CASE("Write batch handles entries independently")
{
    test::CommandFixture fixture;
    const auto id = fixture.instance.GetInstanceId();
    auto invalid = core::StatePath::Instance(id);
    invalid.field = core::StateField::DspParameter;
    const auto& result = Write(fixture, 201, test::Entries({
        test::Write(test::DevicePath(
            id, dsp::DeviceId::MainInputGain, dsp::ParameterId::Gain), 2.0f),
        test::Write(invalid, true),
        test::Write(test::DevicePath(
            id, dsp::DeviceId::MainOutputGain, dsp::ParameterId::Gain), -20.0f)}));

    EXPECT_EQ(result.response.entries.size, 3U);
    EXPECT_EQ(result.response.entries.entries[0].status,
              core::StateWriteStatus::Applied);
    EXPECT_EQ(result.response.entries.entries[1].status,
              core::StateWriteStatus::Rejected);
    EXPECT_EQ(result.response.entries.entries[2].status,
              core::StateWriteStatus::Applied);
}

TEST_CASE("Write command distinguishes unchanged and rejected values")
{
    test::CommandFixture fixture;
    const auto id = fixture.instance.GetInstanceId();
    const auto path = test::DevicePath(
        id, dsp::DeviceId::MainInputGain, dsp::ParameterId::Gain);
    const auto& unchanged = Write(fixture, 202, test::Entries({
        test::Write(path, 0.0f)}));
    EXPECT_EQ(unchanged.response.entries.entries[0].status,
              core::StateWriteStatus::Unchanged);

    const auto& rejected = Write(fixture, 203, test::Entries({
        test::Write(path, 100.0f)}));
    EXPECT_TRUE(rejected.response.entries.entries[0].status.has_value());
    EXPECT_EQ(static_cast<int>(*rejected.response.entries.entries[0].status),
              static_cast<int>(core::StateWriteStatus::Rejected));
}

TEST_CASE("Topology writes expose audibility effects")
{
    test::CommandFixture fixture;
    const auto id = fixture.instance.GetInstanceId();
    const auto& mute = Write(fixture, 204, test::Entries({
        test::Write(core::StatePath::InstanceMute(id), true)}));
    EXPECT_TRUE(mute.effects.audibilityChanged);
    EXPECT_TRUE(fixture.instance.GetStateStore().GetInstance().audibility.mute.value);

    const auto& group = Write(fixture, 205, test::Entries({
        test::Write(core::StatePath::BankGroup(id, dsp::BankId::Bank2),
                    core::GroupId{77})}));
    EXPECT_TRUE(group.effects.audibilityChanged);
    EXPECT_TRUE(group.effects.registryChanged);
    EXPECT_EQ(fixture.registry.FindGroupMembers(core::GroupId{77}).size(), 1U);
}

TEST_CASE("Registry effects distinguish labels from audibility")
{
    test::CommandFixture fixture;
    const auto id = fixture.instance.GetInstanceId();
    const auto& label = Write(fixture, 207, test::Entries({
        test::Write(core::StatePath::Label(id), std::string{"Lead"})}));

    EXPECT_TRUE(label.effects.registryChanged);
    EXPECT_FALSE(label.effects.audibilityChanged);
}

TEST_CASE("Write command for missing instance produces no response")
{
    test::CommandFixture fixture;
    const auto result = fixture.commandRouter.HandleCommand(core::WriteStateCommand{
        .requestId = 206,
        .instanceId = core::InstanceId{999}});
    EXPECT_TRUE(std::holds_alternative<core::NoCommandResponse>(result));
}

TEST_MAIN()

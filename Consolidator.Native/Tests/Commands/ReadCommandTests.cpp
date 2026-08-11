#include "Support/CommandFixture.h"
#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <variant>

using namespace consolidator;

TEST_CASE("Read command with no query returns the complete instance snapshot")
{
    test::CommandFixture fixture;
    const auto id = fixture.instance.GetInstanceId();
    const auto result = fixture.commandRouter.HandleCommand(core::ReadStateCommand{
        .requestId = 100,
        .instanceId = id});

    EXPECT_TRUE(std::holds_alternative<core::StateResponse>(result));
    const auto& response = std::get<core::StateResponse>(result);
    EXPECT_EQ(response.requestId, 100U);
    EXPECT_EQ(response.instanceId, id);
    EXPECT_TRUE(response.entries.size > 256U);
    EXPECT_FALSE(response.truncated);
}

TEST_CASE("Read command overrides query instance with its envelope instance")
{
    test::CommandFixture fixture;
    const auto id = fixture.instance.GetInstanceId();
    auto queries = test::Entries({test::Write(
        test::DevicePath(core::InstanceId{999}, dsp::DeviceId::Compressor,
                         dsp::ParameterId::Ratio), std::monostate{})});
    const auto result = fixture.commandRouter.HandleCommand(core::ReadStateCommand{
        .requestId = 101,
        .instanceId = id,
        .queries = queries});

    const auto& response = std::get<core::StateResponse>(result);
    EXPECT_EQ(response.entries.size, 1U);
    EXPECT_EQ(response.entries.entries[0].path.instanceId, id);
    EXPECT_EQ(std::get<float>(response.entries.entries[0].value), 4.0f);
    EXPECT_TRUE(response.entries.entries[0].minimum.has_value());
}

TEST_CASE("Read command preserves query order and supports topology")
{
    test::CommandFixture fixture;
    const auto id = fixture.instance.GetInstanceId();
    const auto queries = test::Entries({
        test::Write(core::StatePath::SelectedBank(id), std::monostate{}),
        test::Write(core::StatePath::InstanceMute(id), std::monostate{})});
    const auto result = fixture.commandRouter.HandleCommand(core::ReadStateCommand{
        .requestId = 102, .instanceId = id, .queries = queries});
    const auto& response = std::get<core::StateResponse>(result);

    EXPECT_EQ(response.entries.size, 2U);
    EXPECT_EQ(std::get<dsp::BankId>(response.entries.entries[0].value),
              dsp::BankId::Bank0);
    EXPECT_FALSE(std::get<bool>(response.entries.entries[1].value));
}

TEST_CASE("Read command for missing instance produces no response")
{
    test::CommandFixture fixture;
    const auto result = fixture.commandRouter.HandleCommand(core::ReadStateCommand{
        .requestId = 103,
        .instanceId = core::InstanceId{999}});
    EXPECT_TRUE(std::holds_alternative<core::NoCommandResponse>(result));
}

TEST_MAIN()

#include "Dsp/Processors/DspChain.h"
#include "Dsp/Processors/Equalizer/Equalizer.h"
#include "Support/CommandFixture.h"
#include "Support/TestFramework.h"

#include <array>
#include <cstddef>
#include <variant>

using namespace consolidator;

TEST_CASE("Reset command without device route is ignored")
{
    test::CommandFixture fixture;
    const auto result = fixture.commandRouter.HandleCommand(core::ResetDspCommand{
        .requestId = 300,
        .instanceId = fixture.instance.GetInstanceId(),
        .target = core::StatePath::Instance(fixture.instance.GetInstanceId())});
    EXPECT_TRUE(std::holds_alternative<core::ActionResponse>(result));
    const auto& response = std::get<core::ActionResponse>(result);
    EXPECT_EQ(response.requestId, 300U);
    EXPECT_EQ(response.instanceId, fixture.instance.GetInstanceId());
    EXPECT_EQ(response.status, core::ActionStatus::Rejected);
}

TEST_CASE("Reset command is consumed at the next block boundary")
{
    test::CommandFixture fixture;
    const auto id = fixture.instance.GetInstanceId();
    auto& chain = fixture.instance.GetDspChain();
    auto* bank0 = dynamic_cast<dsp::Equalizer*>(chain.GetDevice(3));
    EXPECT_TRUE(bank0 != nullptr);
    auto* filter = bank0->GetFilter(2);
    EXPECT_TRUE(filter->ApplyParameter(
        {dsp::DeviceId::Equalizer, dsp::ParameterId::Gain}, 6.0f, 0));
    filter->CommitRuntimeUpdates();
    (void)filter->ProcessSample(1.0, 0);
    EXPECT_FALSE(filter->GetRuntimeState().channelStates[0].z1 == 0.0);

    core::StatePath target{
        dsp::DeviceId::Equalizer, dsp::ParameterId::Gain,
        dsp::RouteNodeId::Bank0, dsp::RouteNodeId::Filter3};
    const auto result = fixture.commandRouter.HandleCommand(core::ResetDspCommand{
        .requestId = 301, .instanceId = id, .target = target});
    EXPECT_TRUE(std::holds_alternative<core::ActionResponse>(result));
    const auto& response = std::get<core::ActionResponse>(result);
    EXPECT_EQ(response.requestId, 301U);
    EXPECT_EQ(response.instanceId, id);
    EXPECT_EQ(response.status, core::ActionStatus::Accepted);
    EXPECT_FALSE(filter->GetRuntimeState().channelStates[0].z1 == 0.0);

    std::array<double, 2> mainInput{};
    std::array<double, 2> referenceInput{};
    std::array<double, 2> mainOutput{};
    std::array<double, 2> referenceOutput{};
    fixture.instance.Process(mainInput.data(), mainInput.data() + 1,
                             referenceInput.data(), referenceInput.data() + 1,
                             mainOutput.data(), mainOutput.data() + 1,
                             referenceOutput.data(), referenceOutput.data() + 1, 1);
    EXPECT_EQ(filter->GetRuntimeState().channelStates[0].z1, 0.0);
    EXPECT_EQ(filter->GetRuntimeState().channelStates[0].z2, 0.0);
}

TEST_CASE("Reset command for unknown instance has no side effects")
{
    test::CommandFixture fixture;
    const auto result = fixture.commandRouter.HandleCommand(core::ResetDspCommand{
        .requestId = 302,
        .instanceId = core::InstanceId{999},
        .target = core::StatePath::Device(dsp::DeviceId::Compressor)});
    EXPECT_TRUE(std::holds_alternative<core::ActionResponse>(result));
    const auto& response = std::get<core::ActionResponse>(result);
    EXPECT_EQ(response.requestId, 302U);
    EXPECT_EQ(response.instanceId, core::InstanceId{999});
    EXPECT_EQ(response.status, core::ActionStatus::Rejected);
}

TEST_CASE("Reset command rejects an unknown nested route")
{
    test::CommandFixture fixture;
    const auto result = fixture.commandRouter.HandleCommand(core::ResetDspCommand{
        .requestId = 303,
        .instanceId = fixture.instance.GetInstanceId(),
        .target = core::StatePath::Device(dsp::DeviceId::Compressor)
            .WithNode(dsp::RouteNodeId::Filter1)});
    EXPECT_TRUE(std::holds_alternative<core::ActionResponse>(result));
    const auto& response = std::get<core::ActionResponse>(result);
    EXPECT_EQ(response.status, core::ActionStatus::Rejected);
}

TEST_CASE("Reset command rejects realtime queue overflow")
{
    test::CommandFixture fixture;
    const auto id = fixture.instance.GetInstanceId();
    const auto target = core::StatePath::Device(dsp::DeviceId::Compressor);

    for (std::size_t index = 0; index < 16; ++index)
    {
        const auto result = fixture.commandRouter.HandleCommand(core::ResetDspCommand{
            .requestId = static_cast<core::RequestId>(304 + index),
            .instanceId = id,
            .target = target});
        EXPECT_EQ(std::get<core::ActionResponse>(result).status,
                  core::ActionStatus::Accepted);
    }

    const auto result = fixture.commandRouter.HandleCommand(core::ResetDspCommand{
        .requestId = 320,
        .instanceId = id,
        .target = target});
    EXPECT_EQ(std::get<core::ActionResponse>(result).status,
              core::ActionStatus::Rejected);
}

TEST_MAIN()

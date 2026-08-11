#include "Dsp/Processors/DspChain.h"
#include "Dsp/Processors/Equalizer/Equalizer.h"
#include "Support/CommandFixture.h"
#include "Support/TestFramework.h"

#include <array>
#include <variant>

using namespace consolidator;

TEST_CASE("Reset command without device route is ignored")
{
    test::CommandFixture fixture;
    const auto result = fixture.commandRouter.HandleCommand(core::ResetDspCommand{
        .requestId = 300,
        .instanceId = fixture.instance.GetInstanceId(),
        .target = core::StatePath::Instance(fixture.instance.GetInstanceId())});
    EXPECT_TRUE(std::holds_alternative<core::NoCommandResponse>(result));
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
    EXPECT_TRUE(std::holds_alternative<core::NoCommandResponse>(result));
    EXPECT_FALSE(filter->GetRuntimeState().channelStates[0].z1 == 0.0);

    std::array<double, 2> mainInput{};
    std::array<double, 2> referenceInput{};
    std::array<double, 2> mainOutput{};
    std::array<double, 2> referenceOutput{};
    fixture.instance.Process(mainInput.data(), referenceInput.data(),
                             mainOutput.data(), referenceOutput.data(), 1);
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
    EXPECT_TRUE(std::holds_alternative<core::NoCommandResponse>(result));
}

TEST_MAIN()

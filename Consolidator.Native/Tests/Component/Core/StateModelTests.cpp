#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <variant>

using namespace consolidator;

TEST_CASE("StatePath factories produce exact topology addresses")
{
    const core::InstanceId instanceId{7};
    const auto group = core::StatePath::BankGroup(instanceId, dsp::BankId::Bank3);

    EXPECT_EQ(group.instanceId, instanceId);
    EXPECT_EQ(group.field, core::StateField::GroupId);
    EXPECT_EQ(group.depth, 1U);
    EXPECT_EQ(group.TryGetBankId(), dsp::BankId::Bank3);
    EXPECT_FALSE(group.parameterId.has_value());
}

TEST_CASE("StatePath prefix matching respects every populated segment")
{
    const core::InstanceId instanceId{2};
    const auto candidate = test::FilterPath(
        instanceId, dsp::BankId::Bank1, 4, dsp::ParameterId::Gain);

    EXPECT_TRUE(core::StatePath::Instance(instanceId).Matches(candidate));
    EXPECT_TRUE(core::StatePath::Device(dsp::DeviceId::Equalizer).Matches(candidate));
    EXPECT_TRUE(test::BankPath(
        instanceId, dsp::BankId::Bank1, dsp::ParameterId::Gain).Matches(candidate));
    EXPECT_FALSE(test::BankPath(
        instanceId, dsp::BankId::Bank2, dsp::ParameterId::Gain).Matches(candidate));
    EXPECT_FALSE(test::DevicePath(
        instanceId, dsp::DeviceId::Equalizer, dsp::ParameterId::Q).Matches(candidate));
}

TEST_CASE("StatePath immutable modifiers preserve the original")
{
    const auto original = core::StatePath::Device(dsp::DeviceId::Compressor);
    const auto modified = original
        .WithInstance(core::InstanceId{9})
        .WithParameter(dsp::ParameterId::Ratio)
        .WithNode(dsp::RouteNodeId::Detector);

    EXPECT_FALSE(original.instanceId.has_value());
    EXPECT_FALSE(original.parameterId.has_value());
    EXPECT_EQ(original.depth, 0U);
    EXPECT_EQ(modified.instanceId, core::InstanceId{9});
    EXPECT_EQ(modified.parameterId, dsp::ParameterId::Ratio);
    EXPECT_EQ(modified.depth, 1U);
}

TEST_CASE("FixedStateList reports overflow and can be reused")
{
    core::FixedStateList<2> entries;
    EXPECT_TRUE(entries.TryAppend({}));
    EXPECT_TRUE(entries.TryAppend({}));
    EXPECT_FALSE(entries.TryAppend({}));
    EXPECT_EQ(entries.size, 2U);
    EXPECT_TRUE(entries.truncated);

    entries.Clear();
    EXPECT_EQ(entries.size, 0U);
    EXPECT_FALSE(entries.truncated);
    EXPECT_TRUE(entries.TryAppend({}));
}

TEST_MAIN()

#include "Core/Instance/ConsolidatorInstance.h"
#include "Core/Registry/InstanceRegistry.h"
#include "Core/Registry/RegistryState.h"
#include "Support/TestFramework.h"

using namespace consolidator;

TEST_CASE("RegistryState projects labels selection and group membership")
{
    core::ConsolidatorInstance first;
    core::ConsolidatorInstance second;
    first.GetStateStore().SetInstanceId(core::InstanceId{1});
    second.GetStateStore().SetInstanceId(core::InstanceId{2});
    first.GetStateStore().GetInstance().label = "Kick";
    second.GetStateStore().GetInstance().label = "Bass";
    first.GetStateStore().GetInstance().selectedBankId = dsp::BankId::Bank3;
    first.GetStateStore().GetInstance().banks[1].groupId = core::GroupId{0};
    second.GetStateStore().GetInstance().banks[1].groupId = core::GroupId{0};

    core::InstanceRegistry registry;
    registry.RegisterInstance(core::InstanceId{1}, &first);
    registry.RegisterInstance(core::InstanceId{2}, &second);

    core::RegistryState state;
    EXPECT_TRUE(state.Refresh(registry));
    EXPECT_EQ(state.Get().revision, 1U);
    EXPECT_EQ(state.Get().instances.size(), 2U);
    EXPECT_EQ(state.Get().instances[0].label, "Kick");
    EXPECT_EQ(state.Get().instances[0].selectedBankId, dsp::BankId::Bank3);
    EXPECT_EQ(state.Get().groups.size(), 1U);
    EXPECT_EQ(state.Get().groups[0].groupId, core::GroupId{0});
    EXPECT_EQ(state.Get().groups[0].members.size(), 2U);
    EXPECT_EQ(state.Get().groups[0].members[0].instanceId,
              core::InstanceId{1});
    EXPECT_EQ(state.Get().groups[0].members[0].bankId,
              dsp::BankId::Bank1);
    EXPECT_FALSE(state.Refresh(registry));

    first.GetStateStore().GetInstance().label = "Kick 2";
    EXPECT_TRUE(state.Refresh(registry));
    EXPECT_EQ(state.Get().revision, 2U);
    EXPECT_EQ(state.Get().instances[0].label, "Kick 2");
}

TEST_MAIN()

#include "Core/Instance/ConsolidatorInstance.h"
#include "Core/Registry/InstanceRegistry.h"
#include "Core/Routing/GroupGraph.h"
#include "Core/Routing/InstanceAudibilityResolver.h"
#include "Core/Routing/ParameterConstraintResolver.h"
#include "Core/Routing/StateRouter.h"
#include "Support/StateBuilders.h"
#include "Support/TestFramework.h"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <vector>

using namespace consolidator;

namespace
{

class RoutingFixture
{
public:
    explicit RoutingFixture(std::size_t count)
        : graph(registry)
        , router(registry, graph)
        , constraints(registry, router)
        , audibility(registry, graph)
    {
        for (std::size_t index = 0; index < count; ++index)
        {
            instances.push_back(std::make_unique<core::ConsolidatorInstance>());
            const core::InstanceId id{index + 1};
            instances.back()->GetStateStore().SetInstanceId(id);
            registry.RegisterInstance(id, instances.back().get());
        }
    }

    core::ConsolidatorInstance& At(std::size_t index) { return *instances[index]; }

    void Group(std::size_t instance, dsp::BankId bank, core::GroupId group)
    {
        auto& state = At(instance).GetStateStore().GetInstance();
        state.banks[dsp::detail::ToIndex(bank)].groupId = group;
        registry.CacheBankGroup(
            {state.instanceId, bank}, std::nullopt, group);
    }

    core::InstanceRegistry registry;
    core::GroupGraph graph;
    core::StateRouter router;
    core::ParameterConstraintResolver constraints;
    core::InstanceAudibilityResolver audibility;
    std::vector<std::unique_ptr<core::ConsolidatorInstance>> instances;
};

bool Contains(
    const std::vector<core::BankAddress>& values,
    core::BankAddress expected)
{
    return std::find(values.begin(), values.end(), expected) != values.end();
}

bool OutputEnabled(
    const std::vector<core::RuntimeControlUpdate>& updates,
    core::InstanceId id)
{
    const auto found = std::find_if(updates.begin(), updates.end(), [id](const auto& update)
    {
        return update.target.instanceId == id &&
               update.property == core::RuntimeProperty::OutputEnabled;
    });
    EXPECT_TRUE(found != updates.end());
    return found->value;
}

} // namespace

TEST_CASE("InstanceRegistry tracks live instances and removes cached memberships")
{
    RoutingFixture fixture{2};
    fixture.Group(0, dsp::BankId::Bank1, core::GroupId{12});
    EXPECT_TRUE(fixture.registry.Contains(core::InstanceId{1}));
    EXPECT_EQ(fixture.registry.FindGroupMembers(core::GroupId{12}).size(), 1U);

    fixture.registry.UnregisterInstance(
        core::InstanceId{1}, fixture.At(0).GetStateStore().GetInstance());
    EXPECT_FALSE(fixture.registry.Contains(core::InstanceId{1}));
    EXPECT_TRUE(fixture.registry.FindGroupMembers(core::GroupId{12}).empty());
}

TEST_CASE("GroupGraph keeps direct groups separate and traverses connected groups")
{
    RoutingFixture fixture{4};
    fixture.Group(0, dsp::BankId::Bank0, core::GroupId{100});
    fixture.Group(1, dsp::BankId::Bank0, core::GroupId{100});
    fixture.Group(1, dsp::BankId::Bank1, core::GroupId{101});
    fixture.Group(2, dsp::BankId::Bank0, core::GroupId{101});
    fixture.Group(2, dsp::BankId::Bank1, core::GroupId{102});
    fixture.Group(3, dsp::BankId::Bank0, core::GroupId{102});

    const auto direct = fixture.graph.GetGroupMembers(
        {core::InstanceId{1}, dsp::BankId::Bank0});
    EXPECT_EQ(direct.size(), 2U);
    EXPECT_FALSE(Contains(direct, {core::InstanceId{3}, dsp::BankId::Bank0}));

    const std::array seeds{core::BankAddress{
        core::InstanceId{1}, dsp::BankId::Bank0}};
    const auto connected = fixture.graph.GetConnectedGroupBanks(seeds);
    EXPECT_EQ(connected.size(), 6U);
    EXPECT_TRUE(Contains(connected, {core::InstanceId{4}, dsp::BankId::Bank0}));
}

TEST_CASE("StateRouter uses explicit EQ bank and selected bank for device writes")
{
    RoutingFixture fixture{2};
    fixture.At(0).GetStateStore().GetInstance().selectedBankId = dsp::BankId::Bank3;
    fixture.Group(0, dsp::BankId::Bank3, core::GroupId{5});
    fixture.Group(1, dsp::BankId::Bank2, core::GroupId{5});

    const auto deviceTargets = fixture.router.ResolveWriteTargets(
        core::InstanceId{1}, test::DevicePath(
            core::InstanceId{1}, dsp::DeviceId::Compressor, dsp::ParameterId::Ratio));
    EXPECT_EQ(deviceTargets.size(), 2U);

    const auto localBankTargets = fixture.router.ResolveWriteTargets(
        core::InstanceId{1}, test::FilterPath(
            core::InstanceId{1}, dsp::BankId::Bank0, 0, dsp::ParameterId::Gain));
    EXPECT_EQ(localBankTargets.size(), 1U);
    EXPECT_EQ(localBankTargets[0].bankId, dsp::BankId::Bank0);
}

TEST_CASE("Parameter constraints translate a relative movement for every target")
{
    RoutingFixture fixture{2};
    fixture.Group(0, dsp::BankId::Bank0, core::GroupId{8});
    fixture.Group(1, dsp::BankId::Bank1, core::GroupId{8});
    const auto sourcePath = test::FilterPath(
        core::InstanceId{1}, dsp::BankId::Bank0, 2, dsp::ParameterId::Gain);
    core::StateResponseEntries ignored;
    fixture.At(1).GetStateStore().WriteState(test::Write(
        test::FilterPath(core::InstanceId{2}, dsp::BankId::Bank1, 2,
                         dsp::ParameterId::Gain), 10.0f), ignored);

    const auto request = test::Write(sourcePath, 3.0f);
    EXPECT_TRUE(fixture.constraints.Validate(core::InstanceId{1}, request));
    const auto translated = fixture.constraints.TranslateForTarget(
        core::InstanceId{1}, request,
        {core::InstanceId{2}, dsp::BankId::Bank1});
    EXPECT_TRUE(translated.has_value());
    EXPECT_EQ(std::get<float>(translated->value), 13.0f);

    EXPECT_FALSE(fixture.constraints.Validate(
        core::InstanceId{1}, test::Write(sourcePath, 20.0f)));
}

TEST_CASE("Instance audibility uses direct selected-bank group and mute wins")
{
    RoutingFixture fixture{3};
    fixture.Group(0, dsp::BankId::Bank0, core::GroupId{20});
    fixture.Group(1, dsp::BankId::Bank0, core::GroupId{20});
    fixture.At(0).GetStateStore().GetInstance().audibility.solo.value = true;
    fixture.At(1).GetStateStore().GetInstance().audibility.mute.value = true;

    std::vector<core::RuntimeControlUpdate> updates;
    fixture.audibility.Resolve(updates);
    EXPECT_TRUE(OutputEnabled(updates, core::InstanceId{1}));
    EXPECT_FALSE(OutputEnabled(updates, core::InstanceId{2}));
    EXPECT_FALSE(OutputEnabled(updates, core::InstanceId{3}));
}

TEST_MAIN()

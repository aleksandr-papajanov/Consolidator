#include "Core/Routing/GroupGraph.h"

#include <algorithm>
#include <cassert>
#include <vector>

#include "Core/Coordinator/InstanceCoordinator.h"
#include "Core/Instance/ConsolidatorInstance.h"

namespace
{

using consolidator::core::BankAddress;
using consolidator::core::ConsolidatorInstance;
using consolidator::core::GroupGraph;
using consolidator::core::GroupId;
using consolidator::core::InstanceCoordinator;
using consolidator::dsp::BankId;

bool Contains(
    const std::vector<BankAddress>& addresses,
    BankAddress expected)
{
    return std::find(
               addresses.begin(),
               addresses.end(),
               expected) != addresses.end();
}

void AssertContainsExactly(
    const std::vector<BankAddress>& actual,
    std::initializer_list<BankAddress> expected)
{
    assert(actual.size() == expected.size());

    for (const auto& address : expected)
    {
        assert(Contains(actual, address));
    }
}

void SetGroup(
    ConsolidatorInstance& instance,
    BankId bankId,
    GroupId groupId)
{
    auto& state = instance.GetStateStore().GetInstance();
    auto& bank = state.banks[consolidator::dsp::detail::ToIndex(bankId)];

    const auto previousGroup = bank.groupId;
    bank.groupId = groupId;

    InstanceCoordinator::Get().GetRegistry().CacheBankGroup(
        BankAddress{instance.GetInstanceId(), bankId},
        previousGroup,
        groupId);
}

} // namespace

int main()
{
    ConsolidatorInstance first;
    ConsolidatorInstance second;
    ConsolidatorInstance third;
    ConsolidatorInstance fourth;

    first.Initialize();
    second.Initialize();
    third.Initialize();
    fourth.Initialize();

    const auto firstId = first.GetInstanceId();
    const auto secondId = second.GetInstanceId();
    const auto thirdId = third.GetInstanceId();
    const auto fourthId = fourth.GetInstanceId();

    /*
        Topology:

        Group 100:
            first / Bank0
            second / Bank0

        Group 101:
            second / Bank1
            third / Bank0

        Group 102:
            third / Bank1
            fourth / Bank0

        first -- second -- third -- fourth

        Direct groups must remain separate.
        Connected traversal must reach the whole chain.
    */

    SetGroup(first, BankId::Bank0, GroupId{100});
    SetGroup(second, BankId::Bank0, GroupId{100});

    SetGroup(second, BankId::Bank1, GroupId{101});
    SetGroup(third, BankId::Bank0, GroupId{101});

    SetGroup(third, BankId::Bank1, GroupId{102});
    SetGroup(fourth, BankId::Bank0, GroupId{102});

    const GroupGraph graph{
        InstanceCoordinator::Get().GetRegistry()};

    // ------------------------------------------------------------
    // Direct group
    // ------------------------------------------------------------

    AssertContainsExactly(
        graph.GetGroupMembers(
            BankAddress{firstId, BankId::Bank0}),
        {
            BankAddress{firstId, BankId::Bank0},
            BankAddress{secondId, BankId::Bank0},
        });

    AssertContainsExactly(
        graph.GetGroupMembers(
            BankAddress{secondId, BankId::Bank1}),
        {
            BankAddress{secondId, BankId::Bank1},
            BankAddress{thirdId, BankId::Bank0},
        });

    // A direct group must never leak through overlapping groups.
    const auto direct =
        graph.GetGroupMembers(
            BankAddress{secondId, BankId::Bank0});

    assert(!Contains(
        direct,
        BankAddress{thirdId, BankId::Bank0}));

    assert(!Contains(
        direct,
        BankAddress{fourthId, BankId::Bank0}));

    // ------------------------------------------------------------
    // Ungrouped bank
    // ------------------------------------------------------------

    const auto ungrouped =
        graph.GetGroupMembers(
            BankAddress{firstId, BankId::Bank2});

    assert(ungrouped.empty());

    // ------------------------------------------------------------
    // Grouped banks of one instance
    // ------------------------------------------------------------

    AssertContainsExactly(
        graph.GetGroupedBanks(secondId),
        {
            BankAddress{secondId, BankId::Bank0},
            BankAddress{secondId, BankId::Bank1},
        });

    AssertContainsExactly(
        graph.GetGroupedBanks(thirdId),
        {
            BankAddress{thirdId, BankId::Bank0},
            BankAddress{thirdId, BankId::Bank1},
        });

    // ------------------------------------------------------------
    // Transitive connected component
    // ------------------------------------------------------------

    const std::vector<BankAddress> seeds{
        BankAddress{firstId, BankId::Bank0}};

    const auto connected =
        graph.GetConnectedGroupBanks(seeds);

    AssertContainsExactly(
        connected,
        {
            BankAddress{firstId, BankId::Bank0},

            BankAddress{secondId, BankId::Bank0},
            BankAddress{secondId, BankId::Bank1},

            BankAddress{thirdId, BankId::Bank0},
            BankAddress{thirdId, BankId::Bank1},

            BankAddress{fourthId, BankId::Bank0},
        });

    // ------------------------------------------------------------
    // Traversal works from the middle as well
    // ------------------------------------------------------------

    const std::vector<BankAddress> middleSeeds{
        BankAddress{thirdId, BankId::Bank0}};

    const auto connectedFromMiddle =
        graph.GetConnectedGroupBanks(middleSeeds);

    AssertContainsExactly(
        connectedFromMiddle,
        {
            BankAddress{firstId, BankId::Bank0},

            BankAddress{secondId, BankId::Bank0},
            BankAddress{secondId, BankId::Bank1},

            BankAddress{thirdId, BankId::Bank0},
            BankAddress{thirdId, BankId::Bank1},

            BankAddress{fourthId, BankId::Bank0},
        });

    // ------------------------------------------------------------
    // Multiple seeds must not produce duplicates
    // ------------------------------------------------------------

    const std::vector<BankAddress> duplicateSeeds{
        BankAddress{firstId, BankId::Bank0},
        BankAddress{secondId, BankId::Bank1},
        BankAddress{fourthId, BankId::Bank0}};

    const auto connectedFromMultipleSeeds =
        graph.GetConnectedGroupBanks(duplicateSeeds);

    AssertContainsExactly(
        connectedFromMultipleSeeds,
        {
            BankAddress{firstId, BankId::Bank0},

            BankAddress{secondId, BankId::Bank0},
            BankAddress{secondId, BankId::Bank1},

            BankAddress{thirdId, BankId::Bank0},
            BankAddress{thirdId, BankId::Bank1},

            BankAddress{fourthId, BankId::Bank0},
        });

    return 0;
}

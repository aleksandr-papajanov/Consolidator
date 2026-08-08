#include "Core/Registry/InstanceRegistry.h"
#include "Core/Instance/ConsolidatorInstance.h"

#include <cassert>
#include <vector>

int main()
{
    auto& registry = consolidator::core::InstanceRegistry::Get();

    // --- Register instances ---
    consolidator::core::ConsolidatorInstance instanceA;
    consolidator::core::ConsolidatorInstance instanceB;
    consolidator::core::ConsolidatorInstance instanceC;

    const auto idA = registry.RegisterInstance(&instanceA);
    const auto idB = registry.RegisterInstance(&instanceB);
    const auto idC = registry.RegisterInstance(&instanceC);

    assert(idA != idB);
    assert(idB != idC);

    assert(registry.Contains(idA));
    assert(registry.Contains(idB));
    assert(registry.Contains(idC));

    assert(registry.FindInstance(idA) == &instanceA);
    assert(registry.FindInstance(idB) == &instanceB);
    assert(registry.FindInstance(idC) == &instanceC);

    // FindInstance must return nullptr for a stale id.
    assert(registry.FindInstance(consolidator::core::InstanceId{999}) == nullptr);

    // --- Groups ---
    const std::vector<consolidator::core::InstanceId> groupAIds {idA, idB};
    const auto groupA = registry.CreateGroup(groupAIds);

    const auto* groupAInfo = registry.FindGroup(groupA);
    assert(groupAInfo != nullptr);
    assert(groupAInfo->GetMembers().size() == 2);
    assert(groupAInfo->Contains(idA));
    assert(groupAInfo->Contains(idB));

    // Add instance C to group A.
    registry.AddToGroup(groupA, idC);
    assert(groupAInfo->GetMembers().size() == 3);

    // Remove instance B from group A.
    registry.RemoveFromGroup(groupA, idB);
    assert(groupAInfo->GetMembers().size() == 2);

    // --- Routing: parameter route to a single instance (no crash) ---
    const consolidator::dsp::RoutedParameterChange change{
        consolidator::dsp::ParameterRoute{
            consolidator::dsp::DeviceId::MainInputGain,
            consolidator::dsp::ParameterId::Gain},
        consolidator::dsp::ParameterValue{0.5f}};
    registry.Send(idA, change);
    registry.SendToGroup(groupA, change);

    // --- Unregister: instance removed, auto-removed from groups ---
    registry.UnregisterInstance(idA);
    assert(!registry.Contains(idA));
    assert(registry.FindInstance(idA) == nullptr);

    registry.RemoveGroup(groupA);
    assert(registry.FindGroup(groupA) == nullptr);

    registry.UnregisterInstance(idB);
    registry.UnregisterInstance(idC);

    return 0;
}

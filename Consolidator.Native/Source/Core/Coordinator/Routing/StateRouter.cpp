#include "Core/Coordinator/Routing/StateRouter.h"

#include <cstdint>
#include <algorithm>

#include "Core/State/InstanceState.h"
#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

namespace
{

dsp::RouteNodeId ToRouteNodeId(dsp::BankId bankId) noexcept
{
    return static_cast<dsp::RouteNodeId>(
        static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) +
        dsp::detail::ToIndex(bankId));
}

} // namespace

StateEntry StateRouter::ForBank(StateEntry entry, dsp::BankId bankId)
{
    if (entry.path.depth == 0)
    {
        entry.path.depth = 1;
    }
    entry.path.nodes[0] = ToRouteNodeId(bankId);
    return entry;
}

bool StateRouter::IsBankOwned(const StatePath& path) noexcept
{
    if (!path.deviceId || *path.deviceId != dsp::DeviceId::Equalizer || path.depth == 0)
    {
        return false;
    }

    const auto node = static_cast<std::uint8_t>(path.nodes[0]);
    const auto first = static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0);
    return node >= first && node < first + InstanceState::kBankCount;
}

std::vector<GroupId> StateRouter::ResolveAffectedGroups(
    InstanceId instanceId,
    const StatePath& changedPath) const
{
    const auto* instance = registry_.FindInstance(instanceId);
    if (instance == nullptr)
    {
        return {};
    }

    std::vector<GroupId> groups;
    if (IsBankOwned(changedPath) && changedPath.depth != 0)
    {
        const auto node = static_cast<std::uint8_t>(changedPath.nodes[0]);
        const auto first = static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0);
        if (node >= first && node < first + InstanceState::kBankCount)
        {
            const auto group = instance->GetState().GetBankState(
                static_cast<dsp::BankId>(node - first)).GetGroupId();
            if (group)
            {
                groups.push_back(*group);
            }
        }
        return groups;
    }

    for (std::size_t bankIndex = 0; bankIndex < InstanceState::kBankCount; ++bankIndex)
    {
        const auto group = instance->GetState().GetBankState(
            static_cast<dsp::BankId>(bankIndex)).GetGroupId();
        if (group && std::find(groups.begin(), groups.end(), *group) == groups.end())
        {
            groups.push_back(*group);
        }
    }
    return groups;
}

std::vector<BankAddress> StateRouter::ResolveTargets(
    InstanceId sourceInstanceId,
    const StatePath& path) const
{
    if (!path.deviceId)
    {
        return {};
    }

    const auto* source = registry_.FindInstance(sourceInstanceId);
    if (source == nullptr)
    {
        return {};
    }

    if (IsBankOwned(path))
    {
        const auto node = static_cast<std::uint8_t>(path.nodes[0]);
        const auto first = static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0);
        const auto sourceBankId = static_cast<dsp::BankId>(node - first);
        const auto groupId = source->GetState().GetBankState(sourceBankId).GetGroupId();
        if (!groupId)
        {
            return {BankAddress{sourceInstanceId, sourceBankId}};
        }

        const auto members = registry_.FindGroupMembers(*groupId);
        return {members.begin(), members.end()};
    }

    std::vector<BankAddress> targets;
    for (const auto groupId : ResolveAffectedGroups(sourceInstanceId, path))
    {
        for (const auto& member : registry_.FindGroupMembers(groupId))
        {
            if (std::none_of(targets.begin(), targets.end(), [member](const BankAddress& candidate)
                { return candidate.instanceId == member.instanceId; }))
            {
                targets.push_back(member);
            }
        }
    }
    return targets;
}

} // namespace consolidator::core

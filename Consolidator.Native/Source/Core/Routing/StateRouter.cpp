#include "Core/Routing/StateRouter.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include "Core/Domain/State/InstanceState.h"
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

std::vector<BankAddress> StateRouter::ResolveWriteTargets(
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

    std::vector<BankAddress> seeds;
    if (IsBankOwned(path))
    {
        const auto node = static_cast<std::uint8_t>(path.nodes[0]);
        const auto first = static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0);
        const auto sourceBankId = static_cast<dsp::BankId>(node - first);
        seeds.push_back(BankAddress{sourceInstanceId, sourceBankId});
    }
    else
    {
        const auto& topology = source->GetStateStore().GetTopology();
        const auto selectedBankId = topology.GetSelectedBankId();
        if (topology.GetBankState(selectedBankId).GetGroupId())
        {
            seeds.push_back(BankAddress{sourceInstanceId, selectedBankId});
        }
    }

    auto targets = ResolveConnectedComponent(std::move(seeds));
    if (IsBankOwned(path))
    {
        return targets;
    }

    std::vector<BankAddress> uniqueInstanceTargets;
    for (const auto& target : targets)
    {
        const auto alreadyIncluded = std::find_if(
            uniqueInstanceTargets.begin(),
            uniqueInstanceTargets.end(),
            [target](const BankAddress& candidate)
            {
                return candidate.instanceId == target.instanceId;
            });
        if (alreadyIncluded == uniqueInstanceTargets.end())
        {
            uniqueInstanceTargets.push_back(target);
        }
    }
    return uniqueInstanceTargets;
}

std::vector<BankAddress> StateRouter::ResolveConstraintDependencies(
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

    std::vector<BankAddress> seeds;
    if (IsBankOwned(path))
    {
        const auto node = static_cast<std::uint8_t>(path.nodes[0]);
        const auto first = static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0);
        seeds.push_back(BankAddress{
            sourceInstanceId,
            static_cast<dsp::BankId>(node - first)});
    }
    else
    {
        for (std::size_t bankIndex = 0;
             bankIndex < InstanceState::kBankCount;
             ++bankIndex)
        {
            const auto bankId = static_cast<dsp::BankId>(bankIndex);
            if (source->GetStateStore().GetTopology().GetBankState(bankId).GetGroupId())
            {
                seeds.push_back(BankAddress{sourceInstanceId, bankId});
            }
        }
    }

    return ResolveConnectedComponent(std::move(seeds), true);
}

std::vector<BankAddress> StateRouter::ResolveConnectedComponent(
    std::vector<BankAddress> pending,
    bool expandInstanceGroups) const
{
    std::vector<BankAddress> targets;
    for (std::size_t pendingIndex = 0;
         pendingIndex < pending.size();
         ++pendingIndex)
    {
        const auto address = pending[pendingIndex];
        if (std::find(targets.begin(), targets.end(), address) != targets.end())
        {
            continue;
        }
        targets.push_back(address);

        const auto* instance = registry_.FindInstance(address.instanceId);
        if (instance == nullptr)
        {
            continue;
        }

        if (expandInstanceGroups)
        {
            const auto& topology = instance->GetStateStore().GetTopology();
            for (std::size_t bankIndex = 0;
                 bankIndex < InstanceState::kBankCount;
                 ++bankIndex)
            {
                const auto bankId = static_cast<dsp::BankId>(bankIndex);
                if (topology.GetBankState(bankId).GetGroupId())
                {
                    pending.push_back(BankAddress{address.instanceId, bankId});
                }
            }
        }

        const auto groupId = instance->GetStateStore().GetTopology()
            .GetBankState(address.bankId).GetGroupId();
        if (!groupId)
        {
            continue;
        }
        const auto members = registry_.FindGroupMembers(*groupId);
        pending.insert(pending.end(), members.begin(), members.end());
    }
    return targets;
}

std::vector<StatePath> StateRouter::ResolveTopologyConstraintDependencies(
    const std::vector<BankAddress>& affectedBanks) const
{
    std::vector<StatePath> dependencies;
    for (const auto& bank : affectedBanks)
    {
        const auto path = [&bank]
        {
            auto result = StatePath::Instance(bank.instanceId);
            result.field = StateField::DspParameter;
            return result;
        }();
        if (std::find(dependencies.begin(), dependencies.end(), path) == dependencies.end())
        {
            dependencies.push_back(path);
        }
    }
    return dependencies;
}

} // namespace consolidator::core

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

StatePath StateRouter::ForBank(StatePath path, dsp::BankId bankId)
{
    if (path.depth == 0)
    {
        path.depth = 1;
    }
    path.nodes[0] = ToRouteNodeId(bankId);
    return path;
}

StateEntry StateRouter::ForBank(StateEntry entry, dsp::BankId bankId)
{
    entry.path = ForBank(std::move(entry.path), bankId);
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
        if (const auto sourceBankId = path.TryGetBankId())
        {
            seeds.push_back(BankAddress{sourceInstanceId, *sourceBankId});
        }
    }
    else
    {
        const auto& topology = source->GetStateStore().GetInstance();
        const auto selectedBankId = topology.selectedBankId;
        if (topology.banks[dsp::detail::ToIndex(selectedBankId)].groupId)
        {
            seeds.push_back(BankAddress{sourceInstanceId, selectedBankId});
        }
    }

    auto targets = ResolveDirectGroup(std::move(seeds));
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
            if (source->GetStateStore().GetInstance().banks[
                    dsp::detail::ToIndex(bankId)].groupId)
            {
                seeds.push_back(BankAddress{sourceInstanceId, bankId});
            }
        }
    }

    return ResolveConstraintComponent(std::move(seeds));
}

std::vector<BankAddress> StateRouter::ResolveDirectGroup(
    std::vector<BankAddress> seeds) const
{
    return TraverseConnectedComponent(std::move(seeds), false);
}

std::vector<BankAddress> StateRouter::ResolveConstraintComponent(
    std::vector<BankAddress> seeds) const
{
    return TraverseConnectedComponent(std::move(seeds), true);
}

std::vector<BankAddress> StateRouter::TraverseConnectedComponent(
    std::vector<BankAddress> pending,
    bool includeInstanceBanks) const
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

        if (includeInstanceBanks)
        {
            const auto& topology = instance->GetStateStore().GetInstance();
            for (std::size_t bankIndex = 0;
                 bankIndex < InstanceState::kBankCount;
                 ++bankIndex)
            {
                const auto bankId = static_cast<dsp::BankId>(bankIndex);
                if (topology.banks[dsp::detail::ToIndex(bankId)].groupId)
                {
                    pending.push_back(BankAddress{address.instanceId, bankId});
                }
            }
        }

        const auto groupId = instance->GetStateStore().GetInstance()
            .banks[dsp::detail::ToIndex(address.bankId)].groupId;
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

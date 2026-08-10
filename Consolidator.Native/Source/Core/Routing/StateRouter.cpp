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

StatePath StateRouter::Retarget(StatePath path, BankAddress target)
{
    if (IsBankScoped(path))
    {
        path = ForBank(std::move(path), target.bankId);
    }
    path.instanceId = target.instanceId;
    return path;
}

bool StateRouter::IsBankScoped(const StatePath& path) noexcept
{
    return path.deviceId == dsp::DeviceId::Equalizer &&
        path.TryGetBankId().has_value();
}

std::vector<BankAddress> StateRouter::ResolveWriteTargets(
    InstanceId sourceInstanceId,
    const StatePath& path) const
{
    const auto sourceBank = ResolveSourceBank(sourceInstanceId, path);
    if (!sourceBank)
    {
        return {};
    }

    auto targets = GetDirectTargets(*sourceBank);
    if (IsBankScoped(path))
    {
        return targets;
    }

    return CollapseTargetsByInstance(targets);
}

std::vector<BankAddress> StateRouter::ResolveConstraintTargets(
    InstanceId sourceInstanceId,
    const StatePath& path) const
{
    if (IsBankScoped(path))
    {
        const auto sourceBank = ResolveSourceBank(sourceInstanceId, path);
        return sourceBank ? GetDirectTargets(*sourceBank)
                          : std::vector<BankAddress>{};
    }

    const auto seeds = groups_.GetGroupedBanks(sourceInstanceId);
    if (seeds.empty())
    {
        return {};
    }

    return groups_.GetConnectedGroupBanks(seeds);
}

std::vector<BankAddress> StateRouter::CollapseTargetsByInstance(
    const std::vector<BankAddress>& targets) const
{
    std::vector<BankAddress> collapsed;
    for (const auto& target : targets)
    {
        const auto existing = std::find_if(
            collapsed.begin(),
            collapsed.end(),
            [target](const BankAddress& candidate)
            {
                return candidate.instanceId == target.instanceId;
            });
        if (existing == collapsed.end())
        {
            collapsed.push_back(target);
        }
    }
    return collapsed;
}

std::vector<BankAddress> StateRouter::GetDirectTargets(
    BankAddress source) const
{
    auto targets = groups_.GetGroupMembers(source);
    if (targets.empty())
    {
        targets.push_back(source);
    }
    return targets;
}

std::optional<BankAddress> StateRouter::ResolveSourceBank(
    InstanceId instanceId,
    const StatePath& path) const
{
    if (!path.deviceId)
    {
        return std::nullopt;
    }

    const auto* instance = registry_.FindInstance(instanceId);
    if (instance == nullptr)
    {
        return std::nullopt;
    }

    if (IsBankScoped(path))
    {
        const auto bankId = path.TryGetBankId();
        return bankId ? std::optional<BankAddress>{BankAddress{instanceId, *bankId}}
                      : std::nullopt;
    }

    const auto& topology = instance->GetStateStore().GetInstance();
    const auto selectedBankId = topology.selectedBankId;
    return BankAddress{instanceId, selectedBankId};
}

} // namespace consolidator::core

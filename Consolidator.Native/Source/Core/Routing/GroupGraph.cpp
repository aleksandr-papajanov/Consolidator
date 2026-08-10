#include "Core/Routing/GroupGraph.h"

#include <algorithm>

#include "Core/Domain/State/InstanceState.h"
#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

std::vector<BankAddress> GroupGraph::GetGroupMembers(BankAddress bank) const
{
    const auto* instance = registry_.FindInstance(bank.instanceId);
    if (instance == nullptr)
    {
        return {};
    }

    const auto groupId = instance->GetStateStore().GetInstance().banks[
        dsp::detail::ToIndex(bank.bankId)].groupId;
    if (!groupId)
    {
        return {};
    }

    const auto members = registry_.FindGroupMembers(*groupId);
    return {members.begin(), members.end()};
}

std::vector<BankAddress> GroupGraph::GetConnectedGroupBanks(
    std::span<const BankAddress> seeds) const
{
    std::vector<BankAddress> pending{seeds.begin(), seeds.end()};
    std::vector<BankAddress> connected;
    for (std::size_t pendingIndex = 0;
         pendingIndex < pending.size();
         ++pendingIndex)
    {
        const auto bank = pending[pendingIndex];
        if (std::find(connected.begin(), connected.end(), bank) != connected.end())
        {
            continue;
        }
        connected.push_back(bank);

        const auto groupedBanks = GetGroupedBanks(bank.instanceId);
        pending.insert(pending.end(), groupedBanks.begin(), groupedBanks.end());

        const auto members = GetGroupMembers(bank);
        pending.insert(pending.end(), members.begin(), members.end());
    }
    return connected;
}

std::vector<BankAddress> GroupGraph::GetGroupedBanks(InstanceId instanceId) const
{
    const auto* instance = registry_.FindInstance(instanceId);
    if (instance == nullptr)
    {
        return {};
    }

    const auto& topology = instance->GetStateStore().GetInstance();
    std::vector<BankAddress> groupedBanks;
    for (std::size_t bankIndex = 0;
         bankIndex < InstanceState::kBankCount;
         ++bankIndex)
    {
        const auto bankId = static_cast<dsp::BankId>(bankIndex);
        if (topology.banks[dsp::detail::ToIndex(bankId)].groupId)
        {
            groupedBanks.push_back(BankAddress{instanceId, bankId});
        }
    }
    return groupedBanks;
}

} // namespace consolidator::core

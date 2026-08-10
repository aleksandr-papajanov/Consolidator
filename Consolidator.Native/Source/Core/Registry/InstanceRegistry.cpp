#include "Core/Registry/InstanceRegistry.h"

#include <algorithm>

#include "Core/Domain/State/InstanceState.h"

namespace consolidator::core
{

void InstanceRegistry::RegisterInstance(InstanceId instanceId, InstanceHandle instance)
{
    instances_.emplace(instanceId, instance);
}

void InstanceRegistry::UnregisterInstance(InstanceId instanceId, const InstanceState& state)
{
    for (std::size_t bankIndex = 0; bankIndex < InstanceState::kBankCount; ++bankIndex)
    {
        const auto bankId = static_cast<dsp::BankId>(bankIndex);
        CacheBankGroup(BankAddress{instanceId, bankId}, state.GetBankState(bankId).GetGroupId(), std::nullopt);
    }

    instances_.erase(instanceId);
}

InstanceHandle InstanceRegistry::FindInstance(InstanceId instanceId) const noexcept
{
    const auto instanceIt = instances_.find(instanceId);
    return instanceIt != instances_.end() ? instanceIt->second : nullptr;
}

std::span<const BankAddress> InstanceRegistry::FindGroupMembers(GroupId groupId) const noexcept
{
    const auto groupIt = banksByGroup_.find(groupId);
    return groupIt != banksByGroup_.end() ? std::span<const BankAddress>{groupIt->second} : std::span<const BankAddress>{};
}

bool InstanceRegistry::Contains(InstanceId instanceId) const noexcept
{
    return instances_.contains(instanceId);
}

std::vector<InstanceHandle> InstanceRegistry::GetInstances() const
{
    std::vector<InstanceHandle> instances;
    instances.reserve(instances_.size());
    for (const auto& [instanceId, instance] : instances_)
    {
        instances.push_back(instance);
    }
    return instances;
}

void InstanceRegistry::CacheBankGroup(BankAddress bankAddress, std::optional<GroupId> previousGroupId, std::optional<GroupId> nextGroupId)
{
    if (previousGroupId)
    {
        auto groupIt = banksByGroup_.find(*previousGroupId);
        if (groupIt != banksByGroup_.end())
        {
            std::erase_if(groupIt->second, [bankAddress](const BankAddress& candidate)
            {
                return candidate.instanceId == bankAddress.instanceId && candidate.bankId == bankAddress.bankId;
            });
            if (groupIt->second.empty())
            {
                banksByGroup_.erase(groupIt);
            }
        }
    }

    if (nextGroupId)
    {
        auto& members = banksByGroup_[*nextGroupId];
        if (std::find(members.begin(), members.end(), bankAddress) == members.end())
        {
            members.push_back(bankAddress);
        }
    }
}

} // namespace consolidator::core

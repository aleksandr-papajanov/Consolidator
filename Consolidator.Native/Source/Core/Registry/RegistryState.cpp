#include "Core/Registry/RegistryState.h"

#include <algorithm>
#include <map>
#include <utility>

#include "Core/Instance/ConsolidatorInstance.h"
#include "Core/Registry/InstanceRegistry.h"

namespace consolidator::core
{

bool RegistryState::Refresh(const InstanceRegistry& registry)
{
    RegistrySnapshot next;
    for (auto* instance : registry.GetInstances())
    {
        const auto& state = instance->GetStateStore().GetInstance();
        RegistryInstanceSnapshot item;
        item.instanceId = state.instanceId;
        item.label = state.label;
        item.selectedBankId = state.selectedBankId;
        item.banks.reserve(state.banks.size());
        for (const auto& bank : state.banks)
        {
            item.banks.push_back({bank.id, bank.groupId});
        }
        next.instances.push_back(std::move(item));
    }

    std::sort(next.instances.begin(), next.instances.end(),
              [](const auto& left, const auto& right)
              { return left.instanceId.GetValue() < right.instanceId.GetValue(); });

    std::map<GroupId::ValueType, RegistryGroupSnapshot> groups;
    for (const auto& instance : next.instances)
    {
        for (const auto& bank : instance.banks)
        {
            if (bank.groupId)
            {
                groups[bank.groupId->GetValue()].groupId = *bank.groupId;
                groups[bank.groupId->GetValue()].members.push_back(
                    BankAddress{instance.instanceId, bank.bankId});
            }
        }
    }
    for (auto& [groupId, group] : groups)
    {
        (void)groupId;
        std::sort(group.members.begin(), group.members.end(),
                  [](const auto& left, const auto& right)
                  {
                      return std::pair{
                                 left.instanceId.GetValue(),
                                 static_cast<std::uint8_t>(left.bankId)} <
                             std::pair{
                                 right.instanceId.GetValue(),
                                 static_cast<std::uint8_t>(right.bankId)};
                  });
        next.groups.push_back(std::move(group));
    }

    if (next.instances != snapshot_.instances || next.groups != snapshot_.groups)
    {
        next.revision = snapshot_.revision + 1;
        snapshot_ = std::move(next);
        return true;
    }
    return false;
}

} // namespace consolidator::core

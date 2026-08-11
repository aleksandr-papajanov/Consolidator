#include "Core/Routing/InstanceAudibilityResolver.h"

#include <algorithm>

#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

void InstanceAudibilityResolver::Resolve(
    std::vector<RuntimeControlUpdate>& updates) const
{
    updates.clear();

    const auto instances = registry_.GetInstances();
    std::vector<InstanceId> audibleInstances;
    bool hasOutputSolo = false;

    for (const auto* instance : instances)
    {
        if (!instance->GetStateStore().GetInstance().audibility.solo.value)
        {
            continue;
        }

        hasOutputSolo = true;
        const auto instanceId = instance->GetInstanceId();
        audibleInstances.push_back(instanceId);

        const auto selectedBank = instance->GetStateStore().GetInstance().selectedBankId;
        for (const auto& member : groups_.GetGroupMembers(
                 BankAddress{instanceId, selectedBank}))
        {
            if (std::find(
                    audibleInstances.begin(),
                    audibleInstances.end(),
                    member.instanceId) == audibleInstances.end())
            {
                audibleInstances.push_back(member.instanceId);
            }
        }
    }

    for (const auto* instance : instances)
    {
        const bool allowedBySolo = !hasOutputSolo ||
            std::find(
                audibleInstances.begin(),
                audibleInstances.end(),
                instance->GetInstanceId()) != audibleInstances.end();
        const bool enabled =
            !instance->GetStateStore().GetInstance().audibility.mute.value &&
            allowedBySolo;
        updates.push_back(RuntimeControlUpdate{
            StatePath::Instance(instance->GetInstanceId()),
            RuntimeProperty::OutputEnabled,
            enabled,
            0});
    }
}

} // namespace consolidator::core

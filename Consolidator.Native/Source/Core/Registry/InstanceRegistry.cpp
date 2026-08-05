#include "Core/Registry/InstanceRegistry.h"

#include <algorithm>

#include "Core/Instance/ConsolidatorInstance.h"

namespace consolidator::core
{

InstanceRegistry& InstanceRegistry::Get()
{
    static InstanceRegistry instance;
    return instance;
}

InstanceId InstanceRegistry::RegisterInstance(InstanceHandle instance)
{
    const auto instanceId = nextInstanceId_;
    nextInstanceId_ = InstanceId{nextInstanceId_.GetValue() + 1};

    instances_.emplace(instanceId, instance);
    return instanceId;
}

void InstanceRegistry::UnregisterInstance(InstanceId instanceId)
{
    const auto it = groupsByInstance_.find(instanceId);
    if (it != groupsByInstance_.end())
    {
        for (const auto groupId : it->second)
        {
            const auto groupIt = groups_.find(groupId);
            if (groupIt != groups_.end())
            {
                groupIt->second.RemoveMember(instanceId);
                if (groupIt->second.IsEmpty())
                {
                    groups_.erase(groupIt);
                }
            }
        }
        groupsByInstance_.erase(it);
    }

    instances_.erase(instanceId);
}

InstanceHandle InstanceRegistry::FindInstance(InstanceId instanceId) const noexcept
{
    const auto it = instances_.find(instanceId);
    return it != instances_.end() ? it->second : nullptr;
}

bool InstanceRegistry::Contains(InstanceId instanceId) const noexcept
{
    return instances_.find(instanceId) != instances_.end();
}

GroupId InstanceRegistry::CreateGroup(std::span<const InstanceId> members)
{
    const auto groupId = nextGroupId_;
    nextGroupId_ = GroupId{nextGroupId_.GetValue() + 1};

    InstanceGroup group{groupId};
    for (const auto instanceId : members)
    {
        group.AddMember(instanceId);
        groupsByInstance_[instanceId].push_back(groupId);
    }

    groups_.emplace(groupId, std::move(group));
    return groupId;
}

void InstanceRegistry::RemoveGroup(GroupId groupId)
{
    const auto groupIt = groups_.find(groupId);
    if (groupIt == groups_.end())
    {
        return;
    }

    for (const auto member : groupIt->second.GetMembers())
    {
        const auto byInstanceIt = groupsByInstance_.find(member);
        if (byInstanceIt != groupsByInstance_.end())
        {
            std::erase(byInstanceIt->second, groupId);
            if (byInstanceIt->second.empty())
            {
                groupsByInstance_.erase(byInstanceIt);
            }
        }
    }

    groups_.erase(groupIt);
}

void InstanceRegistry::AddToGroup(GroupId groupId, InstanceId instanceId)
{
    const auto groupIt = groups_.find(groupId);
    if (groupIt == groups_.end() || groupIt->second.Contains(instanceId))
    {
        return;
    }

    groupIt->second.AddMember(instanceId);
    groupsByInstance_[instanceId].push_back(groupId);
}

void InstanceRegistry::RemoveFromGroup(GroupId groupId, InstanceId instanceId)
{
    const auto groupIt = groups_.find(groupId);
    if (groupIt == groups_.end())
    {
        return;
    }

    groupIt->second.RemoveMember(instanceId);

    const auto byInstanceIt = groupsByInstance_.find(instanceId);
    if (byInstanceIt != groupsByInstance_.end())
    {
        std::erase(byInstanceIt->second, groupId);
        if (byInstanceIt->second.empty())
        {
            groupsByInstance_.erase(byInstanceIt);
        }
    }

    if (groupIt->second.IsEmpty())
    {
        groups_.erase(groupIt);
    }
}

const InstanceGroup* InstanceRegistry::FindGroup(GroupId groupId) const noexcept
{
    const auto it = groups_.find(groupId);
    return it != groups_.end() ? &it->second : nullptr;
}

void InstanceRegistry::Send(InstanceId instanceId, const dsp::ParameterChange& change)
{
    const auto handle = FindInstance(instanceId);
    if (handle != nullptr)
    {
        handle->ApplyParameterChange(change);
    }
}

void InstanceRegistry::SendToGroup(GroupId groupId, const dsp::ParameterChange& change)
{
    const auto* group = FindGroup(groupId);
    if (group == nullptr)
    {
        return;
    }

    for (const auto member : group->GetMembers())
    {
        Send(member, change);
    }
}

} // namespace consolidator::core

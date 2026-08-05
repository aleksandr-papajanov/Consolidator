#pragma once

#include <algorithm>
#include <vector>

#include "Core/Groups/GroupId.h"
#include "Core/Instance/InstanceId.h"

namespace consolidator::core
{

class InstanceGroup
{
public:
    explicit InstanceGroup(GroupId groupId)
        : groupId_(groupId)
    {
    }

    [[nodiscard]] GroupId GetGroupId() const noexcept
    {
        return groupId_;
    }

    [[nodiscard]] const std::vector<InstanceId>& GetMembers() const noexcept
    {
        return members_;
    }

    void AddMember(InstanceId instanceId)
    {
        members_.push_back(instanceId);
    }

    void RemoveMember(InstanceId instanceId)
    {
        std::erase(members_, instanceId);
    }

    [[nodiscard]] bool Contains(InstanceId instanceId) const noexcept
    {
        return std::find(members_.begin(), members_.end(), instanceId) != members_.end();
    }

    [[nodiscard]] bool IsEmpty() const noexcept
    {
        return members_.empty();
    }

private:
    GroupId groupId_;
    std::vector<InstanceId> members_;
};

} // namespace consolidator::core

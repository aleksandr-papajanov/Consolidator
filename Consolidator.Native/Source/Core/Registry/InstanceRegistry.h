#pragma once

#include <span>
#include <unordered_map>
#include <vector>

#include "Dsp/Parameters/ParameterChange.h"
#include "Core/Groups/GroupId.h"
#include "Core/Groups/InstanceGroup.h"
#include "Core/Instance/InstanceId.h"

namespace consolidator::core
{

class ConsolidatorInstance;

using InstanceHandle = ConsolidatorInstance*;

class InstanceRegistry
{
public:
    static InstanceRegistry& Get();

    InstanceRegistry(const InstanceRegistry&) = delete;
    InstanceRegistry& operator=(const InstanceRegistry&) = delete;

    InstanceId RegisterInstance(InstanceHandle instance);
    void UnregisterInstance(InstanceId instanceId);

    [[nodiscard]] InstanceHandle FindInstance(InstanceId instanceId) const noexcept;
    [[nodiscard]] bool Contains(InstanceId instanceId) const noexcept;

    GroupId CreateGroup(std::span<const InstanceId> members);
    void RemoveGroup(GroupId groupId);

    void AddToGroup(GroupId groupId, InstanceId instanceId);
    void RemoveFromGroup(GroupId groupId, InstanceId instanceId);

    [[nodiscard]] const InstanceGroup* FindGroup(GroupId groupId) const noexcept;

    void Send(InstanceId instanceId, const dsp::ParameterChange& change);
    void SendToGroup(GroupId groupId, const dsp::ParameterChange& change);

private:
    InstanceRegistry() = default;

    InstanceId nextInstanceId_{0};
    GroupId nextGroupId_{0};

    std::unordered_map<InstanceId, InstanceHandle> instances_;
    std::unordered_map<GroupId, InstanceGroup> groups_;
    std::unordered_map<InstanceId, std::vector<GroupId>> groupsByInstance_;
};

} // namespace consolidator::core

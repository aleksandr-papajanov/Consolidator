#pragma once

#include <span>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Core/Groups/GroupId.h"
#include "Core/Instance/InstanceId.h"
#include "Dsp/Parameters/DspIds.h"

namespace consolidator::core
{

class ConsolidatorInstance;
class InstanceState;

using InstanceHandle = ConsolidatorInstance*;

struct BankAddress
{
    InstanceId instanceId;
    dsp::BankId bankId;

    friend bool operator==(const BankAddress&, const BankAddress&) = default;
};

class InstanceRegistry
{
public:
    void RegisterInstance(InstanceId instanceId, InstanceHandle instance);
    void UnregisterInstance(InstanceId instanceId, const InstanceState& state);

    [[nodiscard]] InstanceHandle FindInstance(InstanceId instanceId) const noexcept;
    [[nodiscard]] std::span<const BankAddress> FindGroupMembers(GroupId groupId) const noexcept;
    [[nodiscard]] bool Contains(InstanceId instanceId) const noexcept;

    void CacheBankGroup(BankAddress bankAddress, std::optional<GroupId> previousGroupId, std::optional<GroupId> nextGroupId);

private:
    std::unordered_map<InstanceId, InstanceHandle> instances_;
    std::unordered_map<GroupId, std::vector<BankAddress>> banksByGroup_;
};

} // namespace consolidator::core

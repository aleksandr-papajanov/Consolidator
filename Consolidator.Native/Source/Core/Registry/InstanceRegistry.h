#pragma once

#include <span>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Core/Domain/Ids/GroupId.h"
#include "Core/Domain/Ids/InstanceId.h"
#include "Core/Domain/Ids/DspIds.h"

namespace consolidator::core
{

class ConsolidatorInstance;
struct InstanceState;

using InstanceHandle = ConsolidatorInstance*;

// Identifies a bank together with the instance that owns it.
struct BankAddress
{
    InstanceId instanceId;
    dsp::BankId bankId;

    friend bool operator==(const BankAddress&, const BankAddress&) = default;
};

// Tracks live instances and the reverse index from groups to their banks.
class InstanceRegistry
{
public:
    void RegisterInstance(InstanceId instanceId, InstanceHandle instance);
    void UnregisterInstance(InstanceId instanceId, const InstanceState& state);

    [[nodiscard]] InstanceHandle FindInstance(InstanceId instanceId) const noexcept;
    [[nodiscard]] std::span<const BankAddress> FindGroupMembers(GroupId groupId) const noexcept;
    [[nodiscard]] bool Contains(InstanceId instanceId) const noexcept;
    [[nodiscard]] std::vector<InstanceHandle> GetInstances() const;

    // Keeps group membership lookup synchronized when a bank changes group.
    void CacheBankGroup(BankAddress bankAddress, std::optional<GroupId> previousGroupId, std::optional<GroupId> nextGroupId);

private:
    std::unordered_map<InstanceId, InstanceHandle> instances_;
    std::unordered_map<GroupId, std::vector<BankAddress>> banksByGroup_;
};

} // namespace consolidator::core

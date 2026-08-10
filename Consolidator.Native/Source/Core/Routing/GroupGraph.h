#pragma once

#include <span>
#include <vector>

#include "Core/Registry/InstanceRegistry.h"

namespace consolidator::core
{

// Resolves direct and transitively connected bank groups from the registry.
class GroupGraph final
{
public:
    explicit GroupGraph(const InstanceRegistry& registry) noexcept
        : registry_(registry)
    {
    }

    [[nodiscard]] std::vector<BankAddress> GetGroupMembers(
        BankAddress bank) const;

    [[nodiscard]] std::vector<BankAddress> GetGroupedBanks(
        InstanceId instanceId) const;

    // Traverses the group graph from the supplied banks until the component is complete.
    [[nodiscard]] std::vector<BankAddress> GetConnectedGroupBanks(
        std::span<const BankAddress> seeds) const;

private:
    const InstanceRegistry& registry_;
};

} // namespace consolidator::core

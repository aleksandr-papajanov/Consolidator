#pragma once

#include <vector>

#include "Core/Registry/InstanceRegistry.h"
#include "Core/State/StateProtocol.h"

namespace consolidator::core
{

class StateRouter
{
public:
    explicit StateRouter(const InstanceRegistry& registry) noexcept
        : registry_(registry)
    {
    }

    [[nodiscard]] std::vector<BankAddress> ResolveTargets(
        InstanceId sourceInstanceId,
        const StatePath& path) const;

    [[nodiscard]] static bool IsBankOwned(const StatePath& path) noexcept;

    [[nodiscard]] std::vector<GroupId> ResolveAffectedGroups(
        InstanceId instanceId,
        const StatePath& changedPath) const;

    [[nodiscard]] static StateEntry ForBank(StateEntry entry, dsp::BankId bankId);

private:
    const InstanceRegistry& registry_;
};

} // namespace consolidator::core

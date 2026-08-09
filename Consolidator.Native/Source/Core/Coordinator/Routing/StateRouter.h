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

    [[nodiscard]] std::vector<BankAddress> ResolveWriteTargets(
        InstanceId sourceInstanceId,
        const StatePath& path) const;

    [[nodiscard]] std::vector<BankAddress> ResolveConstraintDependencies(
        InstanceId sourceInstanceId,
        const StatePath& path) const;

    [[nodiscard]] std::vector<StatePath> ResolveTopologyConstraintDependencies(
        const std::vector<BankAddress>& affectedBanks) const;

    [[nodiscard]] static bool IsBankOwned(const StatePath& path) noexcept;

    [[nodiscard]] static StateEntry ForBank(StateEntry entry, dsp::BankId bankId);

private:
    [[nodiscard]] std::vector<BankAddress> ResolveConnectedComponent(
        std::vector<BankAddress> seeds) const;

    const InstanceRegistry& registry_;
};

} // namespace consolidator::core

#pragma once

#include <vector>

#include "Core/Registry/InstanceRegistry.h"
#include "Core/Domain/State/StateEntry.h"

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

    [[nodiscard]] static StatePath ForBank(
        StatePath path,
        dsp::BankId bankId);

    [[nodiscard]] static StateEntry ForBank(StateEntry entry, dsp::BankId bankId);

private:
    [[nodiscard]] std::vector<BankAddress> ResolveDirectGroup(
        std::vector<BankAddress> seeds) const;

    [[nodiscard]] std::vector<BankAddress> ResolveConstraintComponent(
        std::vector<BankAddress> seeds) const;

    [[nodiscard]] std::vector<BankAddress> TraverseConnectedComponent(
        std::vector<BankAddress> seeds,
        bool includeInstanceBanks) const;

    const InstanceRegistry& registry_;
};

} // namespace consolidator::core

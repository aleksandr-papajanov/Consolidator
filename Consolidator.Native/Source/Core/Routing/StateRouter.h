#pragma once

#include <optional>
#include <vector>

#include "Core/Registry/InstanceRegistry.h"
#include "Core/Domain/State/StateEntry.h"
#include "Core/Routing/GroupGraph.h"

namespace consolidator::core
{

// Converts a state path into concrete bank/instance write and constraint targets.
class StateRouter
{
public:
    StateRouter(
        const InstanceRegistry& registry,
        const GroupGraph& groups) noexcept
        : registry_(registry)
        , groups_(groups)
    {
    }

    // Resolves the direct targets affected by a user write.
    [[nodiscard]] std::vector<BankAddress> ResolveWriteTargets(
        InstanceId sourceInstanceId,
        const StatePath& path) const;

    // Resolves all banks whose effective parameter limits must be refreshed.
    [[nodiscard]] std::vector<BankAddress> ResolveConstraintTargets(
        InstanceId sourceInstanceId,
        const StatePath& path) const;

    [[nodiscard]] static bool IsBankScoped(const StatePath& path) noexcept;

    [[nodiscard]] static StatePath ForBank(
        StatePath path,
        dsp::BankId bankId);

    [[nodiscard]] static StateEntry ForBank(StateEntry entry, dsp::BankId bankId);

    // Rewrites a path for a concrete target instance and bank.
    [[nodiscard]] static StatePath Retarget(
        StatePath path,
        BankAddress target);

private:
    [[nodiscard]] std::vector<BankAddress> CollapseTargetsByInstance(
        const std::vector<BankAddress>& targets) const;

    [[nodiscard]] std::vector<BankAddress> GetDirectTargets(
        BankAddress source) const;

    [[nodiscard]] std::optional<BankAddress> ResolveSourceBank(
        InstanceId instanceId,
        const StatePath& path) const;

    const InstanceRegistry& registry_;
    const GroupGraph& groups_;
};

} // namespace consolidator::core

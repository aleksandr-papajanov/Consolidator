#pragma once

#include <optional>
#include <vector>

#include "Core/Registry/InstanceRegistry.h"
#include "Core/Domain/State/StateEntry.h"
#include "Core/Routing/GroupGraph.h"

namespace consolidator::core
{

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

    [[nodiscard]] std::vector<BankAddress> ResolveWriteTargets(
        InstanceId sourceInstanceId,
        const StatePath& path) const;

    [[nodiscard]] std::vector<BankAddress> ResolveConstraintTargets(
        InstanceId sourceInstanceId,
        const StatePath& path) const;

    [[nodiscard]] static bool IsBankScoped(const StatePath& path) noexcept;

    [[nodiscard]] static StatePath ForBank(
        StatePath path,
        dsp::BankId bankId);

    [[nodiscard]] static StateEntry ForBank(StateEntry entry, dsp::BankId bankId);

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

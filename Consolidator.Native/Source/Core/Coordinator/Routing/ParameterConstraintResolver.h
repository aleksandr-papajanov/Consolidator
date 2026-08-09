#pragma once

#include "Core/Coordinator/Routing/StateRouter.h"

namespace consolidator::core
{

// Calculates the effective movement range from the coordinator-owned
// parameter state. The target set is resolved by the existing StateRouter;
// DSP runtime objects are never consulted.
class ParameterConstraintResolver
{
public:
    ParameterConstraintResolver(
        const InstanceRegistry& registry,
        const StateRouter& stateRouter) noexcept
        : registry_(registry)
        , stateRouter_(stateRouter)
    {
    }

    [[nodiscard]] bool Validate(
        InstanceId sourceInstanceId,
        const StateEntry& requested) const;

    [[nodiscard]] std::optional<StateEntry> TranslateForTarget(
        InstanceId sourceInstanceId,
        const StateEntry& requested,
        const BankAddress& target) const;

    void Enrich(
        InstanceId sourceInstanceId,
        StateEntry& entry) const;

private:
    [[nodiscard]] bool ReadParameter(
        InstanceId instanceId,
        const StatePath& path,
        StateEntry& result) const;

    const InstanceRegistry& registry_;
    const StateRouter& stateRouter_;
};

} // namespace consolidator::core

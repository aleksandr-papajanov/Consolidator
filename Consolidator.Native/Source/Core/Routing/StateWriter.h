#pragma once

#include <vector>

#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Instance/Queues/DspUpdateMailbox.h"
#include "Core/Registry/InstanceRegistry.h"
#include "Core/Routing/ParameterConstraintResolver.h"
#include "Core/Routing/StateRouter.h"

namespace consolidator::core
{

class StateWriter final
{
public:
    StateWriter(
        InstanceRegistry& registry,
        const StateRouter& stateRouter,
        const ParameterConstraintResolver& constraintResolver) noexcept;

    [[nodiscard]] StateResponse Write(
        const WriteStateCommand& command);

private:
    struct DspUpdateTarget
    {
        InstanceId instanceId;
        DspUpdate update;
    };

    struct WriteContext
    {
        StateResponse response;
        std::vector<DspUpdateTarget> dspUpdates;
        std::vector<StatePath> constraintPaths;
    };

    void ApplyEntries(
        InstanceId sourceInstanceId,
        const WriteStateCommand& command,
        WriteContext& context);

    void ApplyEntry(
        InstanceId sourceInstanceId,
        const StateEntry& entry,
        WriteContext& context);

    [[nodiscard]] bool TryApplyTopology(
        InstanceId sourceInstanceId,
        const StateEntry& entry,
        WriteContext& context);

    [[nodiscard]] bool ApplyToInstance(
        InstanceId targetInstanceId,
        const StateEntry& entry,
        WriteContext& context);

    void ApplyToTargets(
        InstanceId sourceInstanceId,
        const StateEntry& entry,
        const std::vector<BankAddress>& targets,
        WriteContext& context);

    void CollectConstraintPaths(
        InstanceId targetInstanceId,
        const StateEntry& entry,
        WriteContext& context);

    void AddRejected(
        const StateEntry& entry,
        WriteContext& context) const;

    void PublishDspUpdates(WriteContext& context);
    void RefreshConstraints(WriteContext& context);

    void AppendApplied(
        StateResponseEntries& applied,
        WriteContext& context) const;

    [[nodiscard]] StateResponse FinalizeResponse(WriteContext& context);

    InstanceRegistry& registry_;
    const StateRouter& stateRouter_;
    const ParameterConstraintResolver& constraintResolver_;
};

} // namespace consolidator::core

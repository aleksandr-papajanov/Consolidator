#pragma once

#include <vector>

#include "Core/Domain/Commands/StateProtocolCommands.h"
#include "Core/Instance/Queues/RuntimeUpdateMailbox.h"
#include "Core/Registry/InstanceRegistry.h"
#include "Core/Routing/ParameterConstraintResolver.h"
#include "Core/Routing/ProcessingStateResolver.h"
#include "Core/Routing/StateRouter.h"

namespace consolidator::core
{

// Applies a write batch atomically per entry and publishes its DSP consequences.
struct StateEffects
{
    bool audibilityChanged = false;
    std::vector<InstanceId> analysisInstances;
};

struct StateWriteResult
{
    StateResponse response;
    StateEffects effects;
};

class StateWriter final
{
  public:
    StateWriter(
        InstanceRegistry& registry,
        const StateRouter& stateRouter,
        const ParameterConstraintResolver& constraintResolver) noexcept;

    // Validates, commits, publishes and enriches one protocol write batch.
    [[nodiscard]] StateWriteResult Write(
        const WriteStateCommand& command);

  private:
    struct ParameterUpdateTarget
    {
        InstanceId instanceId;
        ParameterUpdate update;
    };

    struct WriteContext
    {
        StateResponse response;
        std::vector<ParameterUpdateTarget> parameterUpdates;
        std::vector<InstanceId> runtimeInstances;
        std::vector<StatePath> constraintPaths;
        StateEffects effects;
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

    // Delivers all accepted runtime changes after state commits are complete.
    void EnqueueParameterUpdates(WriteContext& context);
    void EnqueueRuntimeUpdates(WriteContext& context);
    // Adds authoritative effective limits affected by the committed writes.
    void RefreshConstraints(WriteContext& context);

    void AppendApplied(
        StateResponseEntries& applied,
        WriteContext& context) const;

    [[nodiscard]] StateResponse FinalizeResponse(WriteContext& context);

    InstanceRegistry& registry_;
    const StateRouter& stateRouter_;
    const ParameterConstraintResolver& constraintResolver_;
    ProcessingStateResolver processingStateResolver_;
};

} // namespace consolidator::core

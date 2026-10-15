using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.Services.PerInstance;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.Commands.Editing;

internal sealed class GainStagingEdits
{
    private readonly InstanceRegistry _instances;
    private readonly IGainStageTelemetry _telemetry;

    public GainStagingEdits(InstanceRegistry instances, IGainStageTelemetry telemetry)
    {
        _instances = instances;
        _telemetry = telemetry;
    }

    public static bool IsLeveler(StatePath path) =>
        IsValue(path, StateNodeIds.InputLeveler) || IsValue(path, StateNodeIds.OutputLeveler);

    public bool IsWriteAllowed(WriteStateCommand command, InstanceCommandContext context)
    {
        if (!command.Entries.Any(entry =>
                IsValue(entry.Path, StateNodeIds.InputLevel) || IsValue(entry.Path, StateNodeIds.InputTarget) ||
                IsValue(entry.Path, StateNodeIds.OutputLevel) || IsValue(entry.Path, StateNodeIds.OutputTarget)))
        {
            return true;
        }
        var targets = ResolveTargets(context, command.WriteMode is WriteScope.Group);
        foreach (var output in new[] { false, true })
        {
            var level = output ? StateNodeIds.OutputLevel : StateNodeIds.InputLevel;
            var target = output ? StateNodeIds.OutputTarget : StateNodeIds.InputTarget;
            var flag = output ? StateNodeIds.OutputLeveler : StateNodeIds.InputLeveler;
            foreach (var instance in targets)
            {
                var state = instance.State.Dsp.GainStaging;
                var enabled = output ? state.OutputLeveler.Value : state.InputLeveler.Value;
                foreach (var entry in command.Entries)
                {
                    if (IsValue(entry.Path, flag) && entry.Value is bool value)
                    {
                        enabled = value;
                    }
                }
                if (command.Entries.Any(entry =>
                        (enabled && IsValue(entry.Path, level)) ||
                        (!enabled && IsValue(entry.Path, target))))
                {
                    return false;
                }
            }
        }
        return true;
    }

    public void PrepareWrite(WriteStateCommand command, InstanceCommandContext context,
        StateHistoryTransaction transaction)
    {
        foreach (var output in new[] { false, true })
        {
            var flag = output ? StateNodeIds.OutputLeveler : StateNodeIds.InputLeveler;
            var level = output ? StateNodeIds.OutputLevel : StateNodeIds.InputLevel;
            if (command.Entries.Any(entry => IsValue(entry.Path, flag) && entry.Value is false) &&
                !command.Entries.Any(entry => IsValue(entry.Path, level)))
            {
                PrepareLevelFreeze(context, command.WriteMode is WriteScope.Group, output, transaction);
            }
        }
    }

    public void PrepareLevelFreeze(InstanceCommandContext context, bool groupScope, bool output,
        StateHistoryTransaction transaction)
    {
        foreach (var target in ResolveTargets(context, groupScope))
        {
            var state = target.State.Dsp.GainStaging;
            var enabled = output ? state.OutputLeveler.Value : state.InputLeveler.Value;
            if (enabled && _telemetry.TryReadSlowLevel(target.InstanceId, output, out var level))
            {
                // Freeze each instance's own slow gain; Native removes the fast overlay through its ramp.
                (output ? state.OutputLevel : state.InputLevel).Prepare(level, transaction);
            }
        }
    }

    private IReadOnlyList<ManagedInstance> ResolveTargets(InstanceCommandContext context, bool groupScope)
    {
        var ids = groupScope && context.BankTarget?.Group is { } group
            ? group.Members.Select(member => member.InstanceId).Distinct().ToArray()
            : new[] { context.InstanceId };
        return ids.Select(id => _instances.FindInstance(id) ??
            throw new InvalidOperationException($"Gain staging target was stopped: {id}.")).ToArray();
    }

    private static bool IsValue(StatePath path, NodeId leaf) =>
        path.Nodes.Count == 3 && path.Nodes[0] == StateNodeIds.Dsp &&
        path.Nodes[1] == StateNodeIds.GainStaging && path.Nodes[2] == leaf;
}

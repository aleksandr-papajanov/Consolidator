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
    private readonly IInputLevelerTelemetry _telemetry;

    public GainStagingEdits(
        InstanceRegistry instances,
        IInputLevelerTelemetry telemetry)
    {
        _instances = instances;
        _telemetry = telemetry;
    }

    public static bool IsInputLeveler(StatePath path) =>
        IsGainStagingValue(path, StateNodeIds.InputLeveler);

    public bool IsWriteAllowed(
        WriteStateCommand command,
        InstanceCommandContext context)
    {
        if (!command.Entries.Any(entry =>
                IsGainStagingValue(entry.Path, StateNodeIds.InputLevel)))
        {
            return true;
        }

        return command.Entries.Any(entry =>
                IsInputLeveler(entry.Path) && entry.Value is false) ||
            ResolveTargets(context, command.WriteMode is WriteScope.Group)
                .All(target => !target.State.Dsp.GainStaging.InputLeveler.Value);
    }

    public void PrepareWrite(
        WriteStateCommand command,
        InstanceCommandContext context,
        StateHistoryTransaction transaction)
    {
        if (command.Entries.Any(entry =>
                IsInputLeveler(entry.Path) && entry.Value is false) &&
            !command.Entries.Any(entry =>
                IsGainStagingValue(entry.Path, StateNodeIds.InputLevel)))
        {
            PrepareInputLevelFreeze(
                context,
                command.WriteMode is WriteScope.Group,
                transaction);
        }
    }

    public void PrepareInputLevelFreeze(
        InstanceCommandContext context,
        bool groupScope,
        StateHistoryTransaction transaction)
    {
        foreach (var target in ResolveTargets(context, groupScope))
        {
            var state = target.State.Dsp.GainStaging;
            if (state.InputLeveler.Value &&
                _telemetry.TryReadSlowLevel(target.InstanceId, out var level))
            {
                // Each instance keeps its own measured level. This derived write
                // must not copy one instance's measurement to its group peers.
                state.InputLevel.Prepare(level, transaction);
            }
        }
    }

    private IReadOnlyList<ManagedInstance> ResolveTargets(
        InstanceCommandContext context,
        bool groupScope)
    {
        var ids = groupScope && context.BankTarget?.Group is { } group
            ? group.Members.Select(member => member.InstanceId).Distinct().ToArray()
            : new[] { context.InstanceId };
        return ids.Select(id => _instances.FindInstance(id) ??
            throw new InvalidOperationException($"Gain staging target was stopped: {id}."))
            .ToArray();
    }

    private static bool IsGainStagingValue(StatePath path, NodeId leaf) =>
        path.Nodes.Count == 3 &&
        path.Nodes[0] == StateNodeIds.Dsp &&
        path.Nodes[1] == StateNodeIds.GainStaging &&
        path.Nodes[2] == leaf;
}

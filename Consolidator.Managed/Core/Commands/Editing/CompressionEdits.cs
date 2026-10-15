using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.Services.PerInstance;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Models.Dsp;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.Commands.Editing;

internal sealed class CompressionEdits
{
    private static readonly NodeId[] ParameterNodes =
        [StateNodeIds.Reduction, StateNodeIds.Output, StateNodeIds.Attack, StateNodeIds.Release, StateNodeIds.Ratio];

    private readonly InstanceRegistry _instances;

    public CompressionEdits(InstanceRegistry instances)
    {
        _instances = instances;
    }

    public static bool IsWriteAllowed(WriteStateCommand command) =>
        command.Entries.All(entry =>
            !IsLeaf(entry.Path, StateNodeIds.Type) || entry.Value is int and >= 0 and < 4);

    public void PrepareWrite(
        WriteStateCommand command,
        InstanceCommandContext context,
        StateHistoryTransaction transaction)
    {
        if (!command.Entries.Any(entry => IsCompressionEntry(entry.Path)))
        {
            return;
        }

        foreach (var target in ResolveTargets(context, command.WriteMode is WriteScope.Group))
        {
            PrepareTarget(target.State.Dsp.Compression, context.State.Dsp.Compression, command.Entries, transaction);
        }
    }

    public void PrepareReset(
        InstanceCommandContext context,
        StatePath path,
        bool groupScope,
        StateHistoryTransaction transaction)
    {
        if (!IsLeaf(path, StateNodeIds.Type) &&
            !ParameterNodes.Any(node => IsLeaf(path, node)))
        {
            return;
        }

        foreach (var target in ResolveTargets(context, groupScope))
        {
            var compression = target.State.Dsp.Compression;
            if (IsLeaf(path, StateNodeIds.Type))
            {
                var defaultType = StateValueDefinitions.Compression.Type.DefaultValue;
                PrepareActiveValues(compression, compression.Profiles[defaultType], transaction);
            }
            else
            {
                var profile = compression.Profiles[compression.Type.Value];
                ProfileValue(profile, path.Nodes[2]).PrepareResetDirect(transaction);
            }
        }
    }

    private static void PrepareTarget(
        CompressionState compression,
        CompressionState source,
        IReadOnlyList<StateWriteEntry> entries,
        StateHistoryTransaction transaction)
    {
        var typeEntry = entries.FirstOrDefault(entry => IsLeaf(entry.Path, StateNodeIds.Type));
        var targetType = typeEntry is null ? compression.Type.Value : (int)typeEntry.Value!;
        var profile = compression.Profiles[targetType];
        foreach (var node in ParameterNodes)
        {
            var entry = entries.FirstOrDefault(candidate => IsLeaf(candidate.Path, node));
            if (entry is not null)
            {
                var value = (float)entry.Value!;
                if (entry.Mode is StateValueEditMode.ApplyDelta && !ReferenceEquals(compression, source))
                {
                    value = ActiveValue(compression, node).Value + (value - ActiveValue(source, node).Value);
                }
                ProfileValue(profile, node).Prepare(value, transaction);
                continue;
            }

            if (typeEntry is not null)
            {
                PrepareActiveValue(compression, profile, node, transaction);
            }
        }
    }

    private static StateValue<float> ProfileValue(CompressionProfile profile, NodeId node) =>
        node == StateNodeIds.Reduction ? profile.Reduction :
        node == StateNodeIds.Output ? profile.OutputDb :
        node == StateNodeIds.Attack ? profile.Attack :
        node == StateNodeIds.Release ? profile.Release :
        node == StateNodeIds.Ratio ? profile.Ratio :
        throw new ArgumentOutOfRangeException(nameof(node));

    private static StateValue<float> ActiveValue(CompressionState compression, NodeId node) =>
        node == StateNodeIds.Reduction ? compression.Reduction :
        node == StateNodeIds.Output ? compression.OutputDb :
        node == StateNodeIds.Attack ? compression.Attack :
        node == StateNodeIds.Release ? compression.Release :
        node == StateNodeIds.Ratio ? compression.Ratio :
        throw new ArgumentOutOfRangeException(nameof(node));

    private static void PrepareActiveValue(
        CompressionState compression,
        CompressionProfile profile,
        NodeId node,
        StateHistoryTransaction transaction)
    {
        ActiveValue(compression, node).Prepare(ProfileValue(profile, node).Value, transaction);
    }

    private static void PrepareActiveValues(
        CompressionState compression,
        CompressionProfile profile,
        StateHistoryTransaction transaction)
    {
        foreach (var node in ParameterNodes)
        {
            PrepareActiveValue(compression, profile, node, transaction);
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
            throw new InvalidOperationException($"Compression target was stopped: {id}."))
            .ToArray();
    }

    private static bool IsCompressionEntry(StatePath path) =>
        path.Nodes.Count >= 3 &&
        path.Nodes[0] == StateNodeIds.Dsp &&
        path.Nodes[1] == StateNodeIds.Compression;

    private static bool IsLeaf(StatePath path, NodeId node) =>
        path.Nodes.Count == 3 &&
        IsCompressionEntry(path) &&
        path.Nodes[2] == node;
}

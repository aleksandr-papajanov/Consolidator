using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.Services;

internal sealed class TargetStateProjector
{
    private readonly StateValueMetadataRegistry _metadata;

    public TargetStateProjector(StateValueMetadataRegistry metadata)
    {
        _metadata = metadata;
    }

    public TargetStateSnapshotResult Project(
        ManagedState state,
        BankId bankId,
        ProcessorId snapshotContext)
    {
        ArgumentNullException.ThrowIfNull(state);

        var values = new List<TargetStateValue>();
        VisitChildren(
            state.Instance.InstanceId,
            state.Root,
            StatePath.Empty,
            bankId,
            snapshotContext,
            values);
        return new TargetStateSnapshotResult(
            state.Instance.InstanceId.Value,
            (int)bankId,
            snapshotContext,
            values);
    }

    private void VisitChildren(
        InstanceId instanceId,
        StateNode node,
        StatePath path,
        BankId bankId,
        ProcessorId snapshotContext,
        ICollection<TargetStateValue> values)
    {
        foreach (var child in node.Children.Values)
        {
            var childPath = path.Append(child.Id);
            if (!ShouldInclude(childPath, bankId, snapshotContext))
            {
                continue;
            }

            if (child.IsContainer)
            {
                VisitChildren(instanceId, child, childPath, bankId, snapshotContext, values);
                continue;
            }

            var reader = new ValueReader();
            child.Accept(reader);
            var metadata = _metadata.Get(instanceId, childPath);
            values.Add(new TargetStateValue(
                childPath,
                reader.Value,
                metadata.PhysicalRange,
                metadata.GetEffectiveRange(new BankAddress(
                    instanceId,
                    (int)bankId))));
        }
    }

    private static bool ShouldInclude(
        StatePath path,
        BankId bankId,
        ProcessorId snapshotContext)
    {
        if (path.Nodes.Count == 1 && path.Nodes[0] == StateNodeIds.Dsp)
        {
            return true;
        }
        if (path.Nodes.Count < 2 || path.Nodes[0] != StateNodeIds.Dsp)
        {
            return false;
        }

        var expectedDevice = snapshotContext switch
        {
            ProcessorId.Input => StateNodeIds.InputGain,
            ProcessorId.Saturator => StateNodeIds.Saturator,
            ProcessorId.Compressor => StateNodeIds.Compressor,
            ProcessorId.Equalizer => StateNodeIds.Equalizer,
            ProcessorId.Polish => StateNodeIds.Polish,
            ProcessorId.Output => StateNodeIds.OutputGain,
            _ => throw new ArgumentOutOfRangeException(nameof(snapshotContext))
        };
        if (path.Nodes[1] != expectedDevice)
        {
            return false;
        }

        var equalizerBankPosition = -1;
        for (var index = 0; index < path.Nodes.Count; index++)
        {
            if (path.Nodes[index] == StateNodeIds.EqualizerBank)
            {
                equalizerBankPosition = index;
                break;
            }
        }
        if (equalizerBankPosition < 0 || path.Nodes.Count <= equalizerBankPosition + 1)
        {
            return true;
        }

        return path.Nodes[equalizerBankPosition + 1] ==
            StateNodeIds.BankAt((int)bankId);
    }

    private sealed class ValueReader : IStateNodeVisitor
    {
        public object? Value { get; private set; }

        public void VisitContainer(StateContainerNode node)
        {
        }

        public void Visit<TValue>(StateNode<TValue> node)
        {
            Value = node.Value;
        }
    }
}

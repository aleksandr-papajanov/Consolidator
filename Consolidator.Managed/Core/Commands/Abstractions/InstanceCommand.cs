using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.Topology;
using Consolidator.Managed.State;
using Consolidator.Managed.Core.Commands.Definitions;

namespace Consolidator.Managed.Core.Commands.Abstractions;

public enum CommandScope
{
    Coordinator,
    FocusedBank
}

public interface IInstanceCommand<TResult>
{
    CommandScope Scope { get; }
}

public interface ITargetedInstanceCommand
{
    InstanceId? TargetInstanceId { get; }
}

public interface IInstanceControlCommand
    : IInstanceCommand<StateWriteStatus>, ITargetedInstanceCommand
{
    InstanceControlScope TargetScope { get; }

    bool RequestedValue { get; }

    InstanceControlSelectionMode Mode { get; }
}

public sealed class InstanceCommandContext
{
    public InstanceCommandContext(
        InstanceId instanceId,
        ManagedState state,
        ContextualBankTarget? bankTarget = null)
    {
        ArgumentNullException.ThrowIfNull(state);

        InstanceId = instanceId;
        State = state;
        BankTarget = bankTarget;
    }

    public InstanceId InstanceId { get; }

    public ManagedState State { get; }

    public ContextualBankTarget? BankTarget { get; }

    public StatePath ResolvePath(StatePath path)
    {
        ArgumentNullException.ThrowIfNull(path);
        if (!path.Nodes.Contains(StateNodeIds.FocusedBank))
        {
            return path;
        }

        var selectedBank = BankTarget?.TargetBank ??
            State.Transient.Selection.SelectedBank ??
            throw new InvalidOperationException(
                "A relative bank path requires a selected bank.");
        return new StatePath(path.Nodes.Select(node =>
            node == StateNodeIds.FocusedBank
                ? StateNodeIds.BankAt(selectedBank.BankIndex)
                : node));
    }
}





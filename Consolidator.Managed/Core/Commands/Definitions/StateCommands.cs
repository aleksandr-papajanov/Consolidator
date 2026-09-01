using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Commands.Definitions;

public sealed record ReadStateCommand(
    StatePath Path,
    InstanceId? TargetInstanceId = null) :
    IInstanceCommand<object?>,
    ITargetedInstanceCommand
{
    public CommandScope Scope => CommandScope.FocusedBank;
}

public sealed record WriteStateCommand(
    IReadOnlyList<StateWriteEntry> Entries,
    ulong TransactionId,
    WriteScope WriteMode,
    InstanceId? TargetInstanceId = null) :
    IInstanceCommand<StateWriteStatus>,
    ITargetedInstanceCommand
{
    public CommandScope Scope => CommandScope.FocusedBank;

    public static WriteStateCommand Create<TValue>(
        StatePath path,
        TValue value,
        StateValueEditMode mode = StateValueEditMode.CopyValue)
    {
        return new WriteStateCommand(
            [new StateWriteEntry(path, value, typeof(TValue), mode)],
            0,
            WriteScope.Local);
    }
}

public enum WriteScope
{
    Local,
    Group,
    Topology
}

public sealed record StateWriteEntry(
    StatePath Path,
    object? Value,
    Type ValueType,
    StateValueEditMode Mode);

public sealed record ResetStateCommand(
    StatePath Target,
    ulong TransactionId,
    ResetScope ResetMode) :
    IInstanceCommand<CommandAcknowledgement>
{
    public CommandScope Scope => CommandScope.FocusedBank;
}

public enum ResetScope
{
    Local,
    Group,
    GroupInstance
}

public sealed record InitializeUiCommand()
    : IInstanceCommand<UiInitializationResult>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record ObserveTargetCommand(
    InstanceId TargetInstanceId,
    BankId BankId,
    ProcessorId SnapshotContext) : IInstanceCommand<TargetStateSnapshotResult>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record SetInstanceActiveCommand(
    bool Active) : IInstanceCommand<CommandAcknowledgement>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record ClearTopologyCommand()
    : IInstanceCommand<StateWriteStatus>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

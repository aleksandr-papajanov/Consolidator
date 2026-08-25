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
    InstanceId? TargetInstanceId = null,
    ulong TransactionId = 0) :
    IInstanceCommand<StateWriteStatus>,
    ITargetedInstanceCommand
{
    public CommandScope Scope => CommandScope.FocusedBank;

    public static WriteStateCommand Create<TValue>(
        StatePath path,
        TValue value)
    {
        return new WriteStateCommand(
            [new StateWriteEntry(path, value, typeof(TValue))]);
    }
}

public sealed record StateWriteEntry(
    StatePath Path,
    object? Value,
    Type ValueType);

public sealed record ResetStateCommand(
    StatePath Target,
    InstanceId? TargetInstanceId,
    ulong TransactionId) :
    IInstanceCommand<CommandAcknowledgement>,
    ITargetedInstanceCommand
{
    public CommandScope Scope => CommandScope.FocusedBank;
}

public sealed record InitializeUiCommand()
    : IInstanceCommand<UiInitializationResult>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record ObserveTargetCommand(
    InstanceId TargetInstanceId,
    BankId BankId) : IInstanceCommand<TargetStateSnapshotResult>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record SetInstanceActiveCommand(
    bool Active) : IInstanceCommand<CommandAcknowledgement>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

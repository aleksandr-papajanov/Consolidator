using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Commands.Definitions;

public sealed record ReadStateCommand(
    StatePath Path) : IInstanceCommand<object?>
{
    public CommandScope Scope => CommandScope.FocusedBank;
}

public sealed record WriteStateCommand(
    StatePath Path,
    object? Value,
    Type ValueType,
    ulong Epoch = 0,
    ulong TransactionId = 0) : IInstanceCommand<StateWriteStatus>
{
    public CommandScope Scope => CommandScope.FocusedBank;

    public static WriteStateCommand Create<TValue>(
        StatePath path,
        TValue value)
    {
        return new WriteStateCommand(
            path,
            value,
            typeof(TValue));
    }
}

public sealed record ResetStateCommand(
    StatePath Target,
    ulong Epoch,
    ulong TransactionId) : IInstanceCommand<CommandAcknowledgement>
{
    public CommandScope Scope => CommandScope.FocusedBank;
}

using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

public sealed class ResetStateCommandHandler
    : CommandHandler<ResetStateCommand, CommandAcknowledgement>
{
    private readonly StateHistory _history;

    public ResetStateCommandHandler(StateHistory history)
    {
        ArgumentNullException.ThrowIfNull(history);
        _history = history;
    }

    public override ValueTask<CommandAcknowledgement> HandleAsync(
        ResetStateCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();

        var node = context.State.Root.Find(command.Target)
            ?? throw new InvalidOperationException(
                $"Reset target was not found: {command.Target}.");
        using var transaction = _history.BeginTransaction();
        var resetCount = node.PrepareResetRecursive(transaction);
        if (resetCount == 0)
        {
            throw new InvalidOperationException(
                $"Reset target contains no resettable state values: {command.Target}.");
        }

        transaction.Commit();

        return ValueTask.FromResult(new CommandAcknowledgement());
    }
}

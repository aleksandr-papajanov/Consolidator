using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Editing;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class ResetStateCommandHandler
    : CommandHandler<ResetStateCommand, CommandAcknowledgement>
{
    private readonly StateHistory _history;
    private readonly GainStagingEdits _gainStaging;

    public ResetStateCommandHandler(
        StateHistory history,
        GainStagingEdits gainStaging)
    {
        ArgumentNullException.ThrowIfNull(history);
        _history = history;
        _gainStaging = gainStaging;
    }

    public override ValueTask<CommandAcknowledgement> HandleAsync(
        ResetStateCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();

        var path = context.ResolvePath(command.Target);
        var node = context.State.Root.Find(path)
            ?? throw new InvalidOperationException(
                $"Reset target was not found: {path}.");
        using var transaction = _history.BeginTransaction();
        if (GainStagingEdits.IsInputLeveler(path))
        {
            _gainStaging.PrepareInputLevelFreeze(
                context,
                command.ResetMode is ResetScope.Group,
                transaction);
        }
        var resetCount = node.PrepareResetRecursive(transaction);
        if (resetCount == 0)
        {
            throw new InvalidOperationException(
                $"Reset target contains no resettable state values: {command.Target}.");
        }

        _history.AdvanceHistoryPoint();
        transaction.Commit();

        return ValueTask.FromResult(new CommandAcknowledgement());
    }
}

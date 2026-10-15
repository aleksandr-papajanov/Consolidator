using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Editing;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.State.History;
using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class ResetStateCommandHandler
    : CommandHandler<ResetStateCommand, CommandAcknowledgement>
{
    private readonly StateHistory _history;
    private readonly GainStagingEdits _gainStaging;
    private readonly CompressionEdits _compression;

    public ResetStateCommandHandler(
        StateHistory history,
        GainStagingEdits gainStaging,
        CompressionEdits compression)
    {
        ArgumentNullException.ThrowIfNull(history);
        _history = history;
        _gainStaging = gainStaging;
        _compression = compression;
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
        if (GainStagingEdits.IsLeveler(path))
        {
            _gainStaging.PrepareLevelFreeze(
                context,
                command.ResetMode is ResetScope.Group,
                path.Nodes[^1] == StateNodeIds.OutputLeveler,
                transaction);
        }
        _compression.PrepareReset(context, path, command.ResetMode is ResetScope.Group, transaction);
        node.PrepareResetRecursive(transaction);
        if (_history.HasHistoryChanges(transaction))
        {
            _history.AdvanceHistoryPoint();
        }
        transaction.Commit();

        return ValueTask.FromResult(new CommandAcknowledgement());
    }
}

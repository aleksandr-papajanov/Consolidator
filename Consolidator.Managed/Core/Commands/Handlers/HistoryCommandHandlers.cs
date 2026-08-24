using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Services.Abstractions;

namespace Consolidator.Managed.Core.Commands.Handlers;

public sealed class BeginHistoryCommandHandler
    : CommandHandler<BeginHistoryCommand, CommandAcknowledgement>
{
    private readonly IHistoryNavigation _history;

    public BeginHistoryCommandHandler(IHistoryNavigation history)
    {
        _history = history;
    }

    public override ValueTask<CommandAcknowledgement> HandleAsync(
        BeginHistoryCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        _history.AdvanceHistoryPoint();
        return ValueTask.FromResult(new CommandAcknowledgement());
    }
}

public sealed class EndHistoryCommandHandler
    : CommandHandler<EndHistoryCommand, CommandAcknowledgement>
{
    public override ValueTask<CommandAcknowledgement> HandleAsync(
        EndHistoryCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return ValueTask.FromResult(new CommandAcknowledgement());
    }
}

public sealed class JumpToHistoryCommandHandler
    : CommandHandler<JumpToHistoryCommand, CommandAcknowledgement>
{
    private readonly IHistoryNavigation _history;

    public JumpToHistoryCommandHandler(IHistoryNavigation history)
    {
        _history = history;
    }

    public override ValueTask<CommandAcknowledgement> HandleAsync(
        JumpToHistoryCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!_history.JumpToHistory(command.Cursor))
        {
            throw new InvalidOperationException("History cursor is out of range.");
        }

        return ValueTask.FromResult(new CommandAcknowledgement());
    }
}

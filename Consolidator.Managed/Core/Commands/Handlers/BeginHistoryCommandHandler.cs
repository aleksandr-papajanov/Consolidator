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

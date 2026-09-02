using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Results;

namespace Consolidator.Managed.Core.Commands.Handlers;

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

using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Services;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class SetInstanceActiveCommandHandler
    : CommandHandler<SetInstanceActiveCommand, CommandAcknowledgement>
{
    private readonly ActiveInstanceCoordinator _activity;

    public SetInstanceActiveCommandHandler(ActiveInstanceCoordinator activity)
    {
        _activity = activity;
    }

    public override ValueTask<CommandAcknowledgement> HandleAsync(
        SetInstanceActiveCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        _activity.SetInstanceActive(context.InstanceId, command.Active);
        return ValueTask.FromResult(new CommandAcknowledgement());
    }
}

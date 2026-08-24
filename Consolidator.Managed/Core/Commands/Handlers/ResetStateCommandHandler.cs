using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.Commands.Handlers;

public sealed class ResetStateCommandHandler
    : CommandHandler<ResetStateCommand, CommandAcknowledgement>
{
    public override ValueTask<CommandAcknowledgement> HandleAsync(
        ResetStateCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();

        var node = context.State.Root.Find(command.Target)
            ?? throw new InvalidOperationException(
                $"Reset target was not found: {command.Target}.");
        var resetCount = node.ResetToInitialRecursive();
        if (resetCount == 0)
        {
            throw new InvalidOperationException(
                $"Reset target contains no resettable state values: {command.Target}.");
        }

        return ValueTask.FromResult(new CommandAcknowledgement());
    }
}

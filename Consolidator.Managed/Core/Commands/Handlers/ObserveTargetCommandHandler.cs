using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Core.Services.Instances;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class ObserveTargetCommandHandler
    : CommandHandler<ObserveTargetCommand, TargetStateSnapshotResult>
{
    private readonly InstanceRegistry _registry;
    private readonly TargetStateProjector _projector;

    public ObserveTargetCommandHandler(
        InstanceRegistry registry,
        TargetStateProjector projector)
    {
        _registry = registry;
        _projector = projector;
    }

    public override ValueTask<TargetStateSnapshotResult> HandleAsync(
        ObserveTargetCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var target = _registry.FindInstance(command.TargetInstanceId)
            ?? throw new KeyNotFoundException(
                $"Observed instance was not found: {command.TargetInstanceId}.");
        if ((int)command.BankId < 0 ||
            (int)command.BankId >= target.State.Instance.Banks.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(command.BankId));
        }

        context.State.Transient.SelectTarget(
            new BankAddress(
                command.TargetInstanceId,
                (int)command.BankId),
            command.SnapshotContext);
        return ValueTask.FromResult(
            _projector.Project(target.State, command.BankId, command.SnapshotContext));
    }
}

using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Accessors;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class SetProcessorBypassCommandHandler
    : CommandHandler<SetProcessorBypassCommand, StateWriteStatus>
{
    private readonly InstanceRegistry _instances;
    private readonly InstanceControlTargetResolver _targets;
    private readonly StateHistory _history;

    public SetProcessorBypassCommandHandler(
        InstanceRegistry instances,
        InstanceControlTargetResolver targets,
        StateHistory history)
    {
        _instances = instances;
        _targets = targets;
        _history = history;
    }

    public override ValueTask<StateWriteStatus> HandleAsync(
        SetProcessorBypassCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!_targets.TryResolve(command.TargetScope, context, out var ids))
        {
            return ValueTask.FromResult(StateWriteStatus.Rejected);
        }

        var values = ids.Select(_instances.FindInstance)
            .Where(instance => instance is not null)
            .Select(instance => ProcessorStateAccess.Bypass(instance!.State, command.ProcessorId))
            .Where(value => value is not null && value.Value != command.Bypassed)
            .Select(value => value!)
            .ToArray();
        if (values.Length == 0) return ValueTask.FromResult(StateWriteStatus.Unchanged);

        using var transaction = _history.BeginTransaction();
        foreach (var value in values) value.Prepare(command.Bypassed, transaction);
        transaction.Commit();
        return ValueTask.FromResult(StateWriteStatus.Applied);
    }
}

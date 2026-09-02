using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.Services.PerInstance;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal abstract class InstanceControlCommandHandler<TCommand>
    : CommandHandler<TCommand, StateWriteStatus>
    where TCommand : IInstanceControlCommand
{
    private readonly InstanceRegistry _instances;
    private readonly InstanceControlTargetResolver _targets;
    private readonly StateHistory _history;

    protected InstanceControlCommandHandler(
        InstanceRegistry instances,
        InstanceControlTargetResolver targets,
        StateHistory history)
    {
        _instances = instances;
        _targets = targets;
        _history = history;
    }

    protected abstract StateValue<bool> GetStateValue(ManagedInstance instance);

    public override ValueTask<StateWriteStatus> HandleAsync(
        TCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (command.TargetInstanceId is not { } targetInstanceId ||
            !_targets.TryResolve(
                command.TargetScope,
                context,
                targetInstanceId,
                out var targetInstanceIds))
        {
            return ValueTask.FromResult(StateWriteStatus.Rejected);
        }

        var instances = _instances.GetInstanceIds()
            .Select(instanceId => _instances.FindInstance(new InstanceId(instanceId)))
            .Where(instance => instance is not null)
            .Select(instance => instance!)
            .ToArray();
        var targetSet = targetInstanceIds.ToHashSet();
        var changes = instances
            .Select(instance =>
            {
                var value = GetStateValue(instance);
                var requested = targetSet.Contains(instance.InstanceId)
                    ? command.RequestedValue
                    : command.RequestedValue && command.Mode is InstanceControlSelectionMode.Exclusive
                        ? false
                        : value.Value;
                return (Value: value, Requested: requested);
            })
            .Where(change => change.Value.Value != change.Requested)
            .ToArray();
        if (changes.Length == 0)
        {
            return ValueTask.FromResult(StateWriteStatus.Unchanged);
        }

        using var transaction = _history.BeginTransaction();
        foreach (var change in changes)
        {
            change.Value.Prepare(change.Requested, transaction);
        }
        transaction.Commit();
        return ValueTask.FromResult(StateWriteStatus.Applied);
    }
}
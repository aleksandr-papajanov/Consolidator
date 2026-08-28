using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class SetInstanceSoloCommandHandler
    : CommandHandler<SetInstanceSoloCommand, StateWriteStatus>
{
    private readonly InstanceRegistry _instances;
    private readonly InstanceControlTargetResolver _targets;
    private readonly StateHistory _history;

    public SetInstanceSoloCommandHandler(
        InstanceRegistry instances,
        InstanceControlTargetResolver targets,
        StateHistory history)
    {
        _instances = instances;
        _targets = targets;
        _history = history;
    }

    public override ValueTask<StateWriteStatus> HandleAsync(
        SetInstanceSoloCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!_targets.TryResolve(command.Target, out var targetInstanceIds))
        {
            return ValueTask.FromResult(StateWriteStatus.Rejected);
        }

        var targetSet = targetInstanceIds.ToHashSet();
        var changes = new List<(StateValue<bool> Value, bool Soloed)>();
        foreach (var instanceIdValue in _instances.GetInstanceIds())
        {
            var instance = _instances.FindInstance(new InstanceId(instanceIdValue));
            if (instance is null)
            {
                continue;
            }

            var requested = targetSet.Contains(instance.InstanceId)
                ? command.Soloed
                : command.Soloed && command.Mode is SoloSelectionMode.Exclusive
                    ? false
                    : instance.State.Instance.Solo.Value;
            if (instance.State.Instance.Solo.Value != requested)
            {
                changes.Add((instance.State.Instance.Solo, requested));
            }
        }

        if (changes.Count == 0)
        {
            return ValueTask.FromResult(StateWriteStatus.Unchanged);
        }

        using var transaction = _history.BeginTransaction();
        foreach (var change in changes)
        {
            change.Value.Prepare(change.Soloed, transaction);
        }
        transaction.Commit();
        return ValueTask.FromResult(StateWriteStatus.Applied);
    }
}

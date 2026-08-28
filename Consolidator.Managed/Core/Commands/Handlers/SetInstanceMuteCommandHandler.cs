using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class SetInstanceMuteCommandHandler
    : CommandHandler<SetInstanceMuteCommand, StateWriteStatus>
{
    private readonly InstanceRegistry _instances;
    private readonly InstanceControlTargetResolver _targets;
    private readonly StateHistory _history;

    public SetInstanceMuteCommandHandler(
        InstanceRegistry instances,
        InstanceControlTargetResolver targets,
        StateHistory history)
    {
        _instances = instances;
        _targets = targets;
        _history = history;
    }

    public override ValueTask<StateWriteStatus> HandleAsync(
        SetInstanceMuteCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (!_targets.TryResolve(command.Target, out var targetInstanceIds))
        {
            return ValueTask.FromResult(StateWriteStatus.Rejected);
        }

        var changes = targetInstanceIds
            .Select(_instances.FindInstance)
            .Where(instance => instance is not null &&
                instance.State.Instance.Mute.Value != command.Muted)
            .Select(instance => instance!.State.Instance.Mute)
            .ToArray();
        if (changes.Length == 0)
        {
            return ValueTask.FromResult(StateWriteStatus.Unchanged);
        }

        using var transaction = _history.BeginTransaction();
        foreach (var value in changes)
        {
            value.Prepare(command.Muted, transaction);
        }
        transaction.Commit();
        return ValueTask.FromResult(StateWriteStatus.Applied);
    }
}

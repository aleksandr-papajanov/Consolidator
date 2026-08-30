using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class ClearTopologyCommandHandler
    : CommandHandler<ClearTopologyCommand, StateWriteStatus>
{
    private readonly InstanceRegistry _instances;
    private readonly StateHistory _history;

    public ClearTopologyCommandHandler(
        InstanceRegistry instances,
        StateHistory history)
    {
        _instances = instances;
        _history = history;
    }

    public override ValueTask<StateWriteStatus> HandleAsync(
        ClearTopologyCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var groups = _instances.GetInstanceIds()
            .Select(id => _instances.FindInstance(new InstanceId(id)))
            .Where(instance => instance is not null)
            .SelectMany(instance => instance!.State.Instance.Banks)
            .Select(bank => bank.Group)
            .Where(group => group.Value is { Value: > 0 })
            .ToArray();
        if (groups.Length == 0)
        {
            return ValueTask.FromResult(StateWriteStatus.Unchanged);
        }

        using var transaction = _history.BeginTransaction();
        foreach (var group in groups)
        {
            group.Prepare(null, transaction);
        }
        transaction.Commit();
        return ValueTask.FromResult(StateWriteStatus.Applied);
    }
}

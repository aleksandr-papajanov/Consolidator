using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class UngroupBanksCommandHandler
    : CommandHandler<UngroupBanksCommand, StateWriteStatus>
{
    private readonly InstanceRegistry _instances;
    private readonly StateHistory _history;

    public UngroupBanksCommandHandler(InstanceRegistry instances, StateHistory history)
    {
        _instances = instances;
        _history = history;
    }

    public override ValueTask<StateWriteStatus> HandleAsync(
        UngroupBanksCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (context.BankTarget is not { Group: { GroupId.Value: > 0 } group } target ||
            target.TargetBank.InstanceId != command.TargetInstanceId ||
            target.TargetBank.BankIndex != command.BankIndex)
        {
            return ValueTask.FromResult(StateWriteStatus.Rejected);
        }

        var values = group.Members.Select(bank =>
            (_instances.FindInstance(bank.InstanceId) ??
                throw new InvalidOperationException($"Topology target was stopped: {bank.InstanceId}."))
            .State.Instance.Banks[bank.BankIndex].Group).ToArray();
        using var transaction = _history.BeginTransaction();
        foreach (var value in values)
        {
            value.Prepare(null, transaction);
        }
        _history.AdvanceHistoryPoint();
        transaction.Commit();
        return ValueTask.FromResult(StateWriteStatus.Applied);
    }
}

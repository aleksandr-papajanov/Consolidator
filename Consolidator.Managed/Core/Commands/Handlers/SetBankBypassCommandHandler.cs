using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class SetBankBypassCommandHandler
    : CommandHandler<SetBankBypassCommand, StateWriteStatus>
{
    private readonly InstanceRegistry _instances;
    private readonly InstanceControlTargetResolver _targets;
    private readonly StateHistory _history;

    public SetBankBypassCommandHandler(
        InstanceRegistry instances,
        InstanceControlTargetResolver targets,
        StateHistory history)
    {
        _instances = instances;
        _targets = targets;
        _history = history;
    }

    public override ValueTask<StateWriteStatus> HandleAsync(
        SetBankBypassCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (command.TargetInstanceId is not { } targetInstanceId ||
            context.BankTarget is not { } bankTarget ||
            bankTarget.TargetBank.InstanceId != targetInstanceId ||
            bankTarget.TargetBank.BankIndex != command.BankIndex ||
            !_targets.TryResolve(
                command.TargetScope,
                context,
                targetInstanceId,
                out var ids))
        {
            return ValueTask.FromResult(StateWriteStatus.Rejected);
        }

        var bankIndex = command.BankIndex;
        var values = ids.Select(_instances.FindInstance)
            .Where(instance => instance is not null)
            .Select(instance => instance!.State.Instance.Banks[bankIndex].Bypass)
            .Where(value => value.Value != command.Bypassed)
            .ToArray();
        if (values.Length == 0) return ValueTask.FromResult(StateWriteStatus.Unchanged);

        using var transaction = _history.BeginTransaction();
        foreach (var value in values) value.Prepare(command.Bypassed, transaction);
        transaction.Commit();
        return ValueTask.FromResult(StateWriteStatus.Applied);
    }
}
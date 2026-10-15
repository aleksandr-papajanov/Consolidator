using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class GroupBanksCommandHandler
    : CommandHandler<GroupBanksCommand, StateWriteStatus>
{
    private readonly InstanceRegistry _instances;
    private readonly StateHistory _history;

    public GroupBanksCommandHandler(InstanceRegistry instances, StateHistory history)
    {
        _instances = instances;
        _history = history;
    }

    public override ValueTask<StateWriteStatus> HandleAsync(
        GroupBanksCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (command.Banks.Count < 2 ||
            command.Banks.Select(bank => bank.InstanceId).Distinct().Count() != command.Banks.Count ||
            command.Banks.Any(bank => bank.BankIndex is < 0 or >= DspConstants.BankCount))
        {
            return ValueTask.FromResult(StateWriteStatus.Rejected);
        }

        var values = new List<StateValue<GroupId?>>();
        foreach (var bank in command.Banks)
        {
            var instance = _instances.FindInstance(bank.InstanceId);
            if (instance is null || instance.State.Instance.Banks[bank.BankIndex].Group.Value is not null)
            {
                return ValueTask.FromResult(StateWriteStatus.Rejected);
            }
            values.Add(instance.State.Instance.Banks[bank.BankIndex].Group);
        }

        var used = _instances.GetInstanceIds()
            .Select(id => _instances.FindInstance(new InstanceId(id))!)
            .SelectMany(instance => instance.State.Instance.Banks)
            .Select(bank => bank.Group.Value)
            .ToHashSet();
        // User groups share the existing sixteen-colour palette; zero is automatic.
        var group = Enumerable.Range(1, 15)
            .Select(index => (GroupId?)new GroupId((uint)index))
            .FirstOrDefault(candidate => !used.Contains(candidate));
        if (group is null)
        {
            return ValueTask.FromResult(StateWriteStatus.Rejected);
        }

        using var transaction = _history.BeginTransaction();
        foreach (var value in values)
        {
            value.Prepare(group, transaction);
        }
        _history.AdvanceHistoryPoint();
        transaction.Commit();
        return ValueTask.FromResult(StateWriteStatus.Applied);
    }
}

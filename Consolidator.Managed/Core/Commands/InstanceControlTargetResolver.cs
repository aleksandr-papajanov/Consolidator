using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.Topology;

namespace Consolidator.Managed.Core.Commands;

internal sealed class InstanceControlTargetResolver
{
    private readonly TopologyIndex _topology;
    private readonly InstanceRegistry _instances;

    public InstanceControlTargetResolver(
        TopologyIndex topology,
        InstanceRegistry instances)
    {
        _topology = topology;
        _instances = instances;
    }

    public bool TryResolve(
        InstanceControlScope scope,
        InstanceCommandContext context,
        out IReadOnlyList<InstanceId> instanceIds)
    {
        var selectedBank = context.State.Transient.Selection.SelectedBank;
        if (selectedBank is null)
        {
            instanceIds = Array.Empty<InstanceId>();
            return false;
        }

        return TryResolve(scope, selectedBank.Value, out instanceIds);
    }

    public bool TryResolve(
        InstanceControlScope scope,
        InstanceCommandContext context,
        InstanceId targetInstanceId,
        out IReadOnlyList<InstanceId> instanceIds)
    {
        ArgumentNullException.ThrowIfNull(context);
        if (context.BankTarget is null ||
            context.BankTarget.TargetBank.InstanceId != targetInstanceId)
        {
            instanceIds = Array.Empty<InstanceId>();
            return false;
        }

        return TryResolve(
            scope,
            context.BankTarget.TargetBank,
            out instanceIds);
    }

    public bool TryResolveBanks(
        InstanceControlScope scope,
        InstanceCommandContext context,
        InstanceId targetInstanceId,
        int bankIndex,
        out IReadOnlyList<BankAddress> banks)
    {
        ArgumentNullException.ThrowIfNull(context);
        if (context.BankTarget is not { } target ||
            target.TargetBank.InstanceId != targetInstanceId ||
            target.TargetBank.BankIndex != bankIndex ||
            !_instances.Contains(targetInstanceId))
        {
            banks = Array.Empty<BankAddress>();
            return false;
        }

        if (scope is InstanceControlScope.Instance)
        {
            banks = [target.TargetBank];
            return true;
        }

        if (scope is not InstanceControlScope.BankGroup)
        {
            banks = Array.Empty<BankAddress>();
            return false;
        }

        banks = target.Group is { Members.Count: > 0 } group
            ? group.Members
            : [target.TargetBank];
        return banks.All(bank => _instances.Contains(bank.InstanceId));
    }

    private bool TryResolve(
        InstanceControlScope scope,
        BankAddress targetBank,
        out IReadOnlyList<InstanceId> instanceIds)
    {
        if (!_instances.Contains(targetBank.InstanceId))
        {
            instanceIds = Array.Empty<InstanceId>();
            return false;
        }

        if (scope is InstanceControlScope.Instance)
        {
            instanceIds = [targetBank.InstanceId];
            return true;
        }

        if (scope is not InstanceControlScope.BankGroup)
        {
            instanceIds = Array.Empty<InstanceId>();
            return false;
        }

        var groupInstanceIds = _topology.GetBankGroupInstanceIds(targetBank);
        instanceIds = groupInstanceIds.Count > 0
            ? groupInstanceIds
            : [targetBank.InstanceId];
        return true;
    }
}

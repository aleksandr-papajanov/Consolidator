using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.Topology;

internal sealed class ContextualBankResolver
{
    private readonly TopologyIndex _topology;

    public ContextualBankResolver(TopologyIndex topology)
    {
        ArgumentNullException.ThrowIfNull(topology);
        _topology = topology;
    }

    public ContextualBankTarget? Resolve(
        InstanceId viewerInstanceId,
        InstanceId targetInstanceId)
    {
        var selectedBank = _topology.ResolveFocusedBankAddress(viewerInstanceId);
        if (selectedBank is null)
        {
            return null;
        }

        var targetBank = new BankAddress(
            targetInstanceId,
            selectedBank.Value.BankIndex);
        var members = _topology.GetConnectedBankPeers(targetBank);
        var group = _topology.TryGetBankGroup(targetBank, out var groupId)
            ? new BankGroupSnapshot(groupId, members)
            : null;
        return new ContextualBankTarget(
            selectedBank.Value,
            targetBank,
            group);
    }
}

public sealed record ContextualBankTarget(
    BankAddress SelectedBank,
    BankAddress TargetBank,
    BankGroupSnapshot? Group);

public sealed record BankGroupSnapshot(
    GroupId GroupId,
    IReadOnlyList<BankAddress> Members);

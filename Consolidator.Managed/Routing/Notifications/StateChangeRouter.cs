using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.Topology;

namespace Consolidator.Managed.Routing.Notifications;

internal sealed class StateChangeRouter
{
    private readonly TopologyIndex _topologyIndex;

    public StateChangeRouter(TopologyIndex topologyIndex)
    {
        ArgumentNullException.ThrowIfNull(topologyIndex);
        _topologyIndex = topologyIndex;
    }

    public IReadOnlyList<ulong> ResolveTargets(StateValueChanged change)
    {
        ArgumentNullException.ThrowIfNull(change);

        if (change.Ownership is StateValueOwnership.InstanceOwned)
        {
            return [change.InstanceId.Value];
        }

        var bank = _topologyIndex.ResolveBankAddress(change.InstanceId, change.Path);
        if (bank is not { } value)
        {
            throw new InvalidOperationException(
                "A bank-owned state change must address a bank in its state path.");
        }

        return _topologyIndex
            .ResolveFocusedInstanceIds(value)
            .Select(instanceId => instanceId.Value)
            .ToArray();
    }

}

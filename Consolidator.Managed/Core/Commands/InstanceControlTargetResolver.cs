using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.State;
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
        InstanceControlTarget target,
        out IReadOnlyList<InstanceId> instanceIds)
    {
        if (target.Scope is InstanceControlScope.Instance)
        {
            instanceIds = [target.InstanceId];
            return target.BankId is null &&
                _instances.Contains(target.InstanceId);
        }

        if (target.Scope is not InstanceControlScope.BankGroup ||
            target.BankId is not { } bankId)
        {
            instanceIds = Array.Empty<InstanceId>();
            return false;
        }

        if (!_instances.Contains(target.InstanceId))
        {
            instanceIds = Array.Empty<InstanceId>();
            return false;
        }

        instanceIds = _topology.GetBankGroupInstanceIds(
            new BankAddress(target.InstanceId, (int)bankId));
        return instanceIds.Count > 0;
    }
}

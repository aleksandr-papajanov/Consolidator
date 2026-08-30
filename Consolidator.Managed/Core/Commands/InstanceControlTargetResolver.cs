using Consolidator.Managed.Core.Commands.Abstractions;
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

        if (scope is InstanceControlScope.Instance)
        {
            instanceIds = [selectedBank.Value.InstanceId];
            return _instances.Contains(selectedBank.Value.InstanceId);
        }

        if (scope is not InstanceControlScope.BankGroup)
        {
            instanceIds = Array.Empty<InstanceId>();
            return false;
        }

        instanceIds = _topology.GetBankGroupInstanceIds(
            selectedBank.Value);
        return instanceIds.Count > 0;
    }
}

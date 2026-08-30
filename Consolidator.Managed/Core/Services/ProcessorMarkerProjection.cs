using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.Topology;

namespace Consolidator.Managed.Core.Services;

internal sealed class ProcessorMarkerProjection
{
    private readonly TopologyIndex _topology;

    public ProcessorMarkerProjection(TopologyIndex topology)
    {
        _topology = topology;
    }

    public RegistrySnapshotResult Project(
        InstanceId viewerInstanceId,
        RegistrySnapshotResult snapshot)
    {
        ArgumentNullException.ThrowIfNull(snapshot);

        var instances = snapshot.Instances.ToDictionary(
            instance => new InstanceId(instance.InstanceId));
        var focusedBank = _topology.ResolveFocusedBankAddress(viewerInstanceId);
        var markers = snapshot.Instances
            .SelectMany(instance => ProjectInstance(instance, instances, focusedBank))
            .ToArray();
        return snapshot with { ProcessorMarkers = markers };
    }

    private IEnumerable<RegistryProcessorMarkerSnapshot> ProjectInstance(
        RegistryInstanceSnapshot instance,
        IReadOnlyDictionary<InstanceId, RegistryInstanceSnapshot> instances,
        BankAddress? focusedBank)
    {
        if (focusedBank is null)
        {
            return instance.Processors.Select(processor =>
                new RegistryProcessorMarkerSnapshot(
                    instance.InstanceId,
                    processor.ProcessorId,
                    processor.EffectActive));
        }

        var rowBank = new BankAddress(
            new InstanceId(instance.InstanceId),
            focusedBank.Value.BankIndex);
        var members = _topology.GetConnectedBankPeers(rowBank);
        return instance.Processors.Select(processor =>
            new RegistryProcessorMarkerSnapshot(
                instance.InstanceId,
                processor.ProcessorId,
                members.Any(member => IsActive(member, processor.ProcessorId, instances))));
    }

    private static bool IsActive(
        BankAddress member,
        ProcessorId processorId,
        IReadOnlyDictionary<InstanceId, RegistryInstanceSnapshot> instances)
    {
        if (!instances.TryGetValue(member.InstanceId, out var instance))
        {
            return false;
        }

        var processor = instance.Processors.FirstOrDefault(
            candidate => candidate.ProcessorId == processorId);
        if (processor is null)
        {
            return false;
        }

        if (processorId != ProcessorId.Equalizer)
        {
            return processor.EffectActive;
        }

        return !processor.Bypassed && instance.Banks.Any(
            bank => bank.BankId == member.BankIndex && bank.EffectActive);
    }
}

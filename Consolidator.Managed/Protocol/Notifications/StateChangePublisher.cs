using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.Topology;
using Consolidator.Managed.Protocol.Encoding;
using Consolidator.Managed.Protocol.Transport;
using Consolidator.Managed.Routing.Notifications;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class StateChangePublisher : IStateChangeSink
{
    private readonly IPresentationTransport _transport;
    private readonly StateChangeRouter _router;
    private readonly IManagedLogger _logger;
    private readonly StateValueMetadataRegistry _metadata;
    private readonly TopologyIndex _topology;
    private readonly RegistryChangePublisher _registryChanges;

    public StateChangePublisher(
        IPresentationTransport transport,
        StateChangeRouter router,
        IManagedLogger logger,
        StateValueMetadataRegistry metadata,
        TopologyIndex topology,
        RegistryChangePublisher registryChanges)
    {
        ArgumentNullException.ThrowIfNull(transport);
        ArgumentNullException.ThrowIfNull(router);
        ArgumentNullException.ThrowIfNull(logger);

        _transport = transport;
        _router = router;
        _logger = logger;
        _metadata = metadata;
        _topology = topology;
        _registryChanges = registryChanges;
    }

    public void Publish(StateValueChanged change)
    {
        ArgumentNullException.ThrowIfNull(change);

        try
        {
            var targets = _router.ResolveTargets(change);
            var bank = _topology.ResolveBankAddress(change.InstanceId, change.Path);
            var metadata = _metadata.Get(change.InstanceId, change.Path);
            if (targets.Count > 0)
            {
                _transport.Send(StateChangeEncoder.Encode(
                    change,
                    targets,
                    metadata,
                    metadata.EffectiveRange,
                    bank?.BankIndex));
            }
            if (change.IsValueChange &&
                change.Path.Nodes.Contains(StateNodeIds.Label))
            {
                _registryChanges.LabelChanged(
                    change.InstanceId.Value,
                    (string)change.CurrentValue!);
            }
            else if (change.IsValueChange &&
                IsInstanceValue(change.Path, StateNodeIds.Mute))
            {
                _registryChanges.InstanceMuteChanged(
                    change.InstanceId.Value,
                    (bool)change.CurrentValue!);
            }
            else if (change.IsValueChange &&
                IsInstanceValue(change.Path, StateNodeIds.Solo))
            {
                _registryChanges.InstanceSoloChanged(
                    change.InstanceId.Value,
                    (bool)change.CurrentValue!);
            }
            else if (change.IsValueChange &&
                change.Path.Nodes.Contains(StateNodeIds.Group) &&
                bank is { } bankAddress)
            {
                _registryChanges.BankGroupChanged(
                    change.InstanceId.Value,
                    bankAddress.BankIndex,
                    change.CurrentValue is GroupId groupId
                        ? groupId.Value
                        : null);
            }
        }
        catch (Exception exception)
        {
            _logger.Warning(
                $"Failed to publish state change for {change.Path}: {exception.Message}");
        }
    }

    private static bool IsInstanceValue(
        Consolidator.Managed.State.StatePath path,
        Consolidator.Managed.State.Tree.NodeId node)
    {
        return path.Nodes.Count == 2 &&
            path.Nodes[0] == StateNodeIds.Instance &&
            path.Nodes[1] == node;
    }
}

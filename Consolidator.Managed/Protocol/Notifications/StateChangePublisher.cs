using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.Topology;
using Consolidator.Managed.Protocol.Encoding;
using Consolidator.Managed.Protocol.Transport;
using Consolidator.Managed.Routing.Notifications;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class StateChangePublisher : IStateChangeSink
{
    private readonly IProtocolTransport _transport;
    private readonly StateChangeRouter _router;
    private readonly IManagedLogger _logger;
    private readonly StateValueMetadataRegistry _metadata;
    private readonly TopologyIndex _topology;
    private readonly RegistryChangePublisher _registryChanges;

    public StateChangePublisher(
        IProtocolTransport transport,
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
            _transport.Send(StateChangeEncoder.Encode(
                change,
                targets,
                _metadata.Get(change.InstanceId, change.Path),
                bank?.BankIndex));
            if (change.Path.Nodes.Contains(StateNodeIds.Label) ||
                change.Path.Nodes.Contains(StateNodeIds.Group))
            {
                _registryChanges.Publish();
            }
        }
        catch (Exception exception)
        {
            _logger.Warning(
                $"Failed to publish state change for {change.Path}: {exception.Message}");
        }
    }
}

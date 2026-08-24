using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Protocol.Encoding;
using Consolidator.Managed.Protocol.Transport;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class HistoryStatePublisher
{
    private readonly IProtocolTransport _transport;
    private readonly InstanceRegistry _instanceRegistry;
    private readonly IManagedLogger _logger;

    public HistoryStatePublisher(
        StateHistory history,
        IProtocolTransport transport,
        InstanceRegistry instanceRegistry,
        IManagedLogger logger)
    {
        ArgumentNullException.ThrowIfNull(history);
        ArgumentNullException.ThrowIfNull(transport);
        ArgumentNullException.ThrowIfNull(instanceRegistry);
        ArgumentNullException.ThrowIfNull(logger);

        _transport = transport;
        _instanceRegistry = instanceRegistry;
        _logger = logger;
        history.Changed += Publish;
    }

    internal void Publish(StateHistorySnapshot snapshot)
    {
        try
        {
            _transport.Send(HistoryStateEncoder.Encode(
                snapshot,
                _instanceRegistry.GetInstanceIds()));
        }
        catch (Exception exception)
        {
            _logger.Warning($"Failed to publish history state: {exception.Message}");
        }
    }
}
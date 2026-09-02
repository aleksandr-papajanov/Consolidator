using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Protocol.Encoding;
using Consolidator.Managed.Protocol.Transport;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class HistoryStatePublisher : IHistoryStateSink
{
    private readonly IPresentationTransport _transport;
    private readonly IManagedLogger _logger;
    private readonly IProtocolOutputRegistry _outputs;

    public HistoryStatePublisher(
        StateHistory history,
        IPresentationTransport transport,
        IManagedLogger logger,
        IProtocolOutputRegistry outputs)
    {
        ArgumentNullException.ThrowIfNull(history);
        ArgumentNullException.ThrowIfNull(transport);
        ArgumentNullException.ThrowIfNull(logger);

        _transport = transport;
        _logger = logger;
        _outputs = outputs;
        history.Changed += Publish;
    }

    public void Publish(StateHistorySnapshot snapshot)
    {
        try
        {
            _transport.Send(HistoryStateEncoder.Encode(
                snapshot,
                _outputs.GetRegisteredInstanceIds()));
        }
        catch (Exception exception)
        {
            _logger.Warning($"Failed to publish history state: {exception.Message}");
        }
    }
}

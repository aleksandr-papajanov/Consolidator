using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Encoding;
using Consolidator.Managed.Protocol.Transport;
using Consolidator.Managed.Routing.Notifications;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class StateChangePublisher : IStateChangeSink
{
    private readonly IProtocolTransport _transport;
    private readonly StateChangeRouter _router;
    private readonly IManagedLogger _logger;

    public StateChangePublisher(
        IProtocolTransport transport,
        StateChangeRouter router,
        IManagedLogger logger)
    {
        ArgumentNullException.ThrowIfNull(transport);
        ArgumentNullException.ThrowIfNull(router);
        ArgumentNullException.ThrowIfNull(logger);

        _transport = transport;
        _router = router;
        _logger = logger;
    }

    public void Publish(StateValueChanged change)
    {
        ArgumentNullException.ThrowIfNull(change);

        try
        {
            var targets = _router.ResolveTargets(change);
            _transport.Send(StateChangeEncoder.Encode(change, targets));
        }
        catch (Exception exception)
        {
            _logger.Warning(
                $"Failed to publish state change for {change.Path}: {exception.Message}");
        }
    }
}

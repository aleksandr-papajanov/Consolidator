using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Dispatch;

internal sealed class CommandEndpointRegistry
{
    private readonly IReadOnlyDictionary<Type, ICommandEndpoint> _endpoints;

    public CommandEndpointRegistry(
        IEnumerable<ICommandEndpoint> endpoints)
    {
        ArgumentNullException.ThrowIfNull(endpoints);

        var endpointMap = new Dictionary<Type, ICommandEndpoint>();
        var selectorMap = new Dictionary<string, Type>(StringComparer.Ordinal);
        foreach (var endpoint in endpoints)
        {
            if (!endpointMap.TryAdd(endpoint.MessageType, endpoint))
            {
                throw new InvalidOperationException(
                    $"Multiple protocol endpoints are registered for {endpoint.MessageType}.");
            }

            if (!selectorMap.TryAdd(endpoint.Selector, endpoint.MessageType))
            {
                throw new InvalidOperationException(
                    $"Multiple protocol endpoints are registered for selector '{endpoint.Selector}'.");
            }
        }

        _endpoints = endpointMap;
    }

    public ValueTask<IReadOnlyList<ProtocolOutput>> ExecuteAsync(
        DecodedCommand decoded,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(decoded);

        if (!_endpoints.TryGetValue(decoded.Command.GetType(), out var endpoint) ||
            !string.Equals(endpoint.Selector, decoded.Selector, StringComparison.Ordinal))
        {
            throw new InvalidOperationException(
                $"No execution endpoint is registered for {decoded.Command.GetType()}.");
        }

        return endpoint.ExecuteAsync(
            decoded.SourceInstanceId,
            decoded.Command,
            decoded.RequestId,
            cancellationToken);
    }
}




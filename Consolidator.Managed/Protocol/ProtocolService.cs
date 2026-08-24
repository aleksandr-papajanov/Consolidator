using Consolidator.Managed.Protocol.Decoding;
using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Encoding;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Protocol;

internal sealed class ProtocolService
{
    private readonly CommandDecoder _decoder;
    private readonly CommandEndpointRegistry _endpoints;
    private readonly IProtocolTransport _transport;

    public ProtocolService(
        CommandDecoder decoder,
        CommandEndpointRegistry endpoints,
        IProtocolTransport transport)
    {
        ArgumentNullException.ThrowIfNull(decoder);
        ArgumentNullException.ThrowIfNull(endpoints);
        ArgumentNullException.ThrowIfNull(transport);

        _decoder = decoder;
        _endpoints = endpoints;
        _transport = transport;
    }

    public void Receive(
        ProtocolInput message)
    {
        ArgumentNullException.ThrowIfNull(message);

        try
        {
            var decoded = _decoder.Decode(message);
            var response = _endpoints
                .ExecuteAsync(decoded, CancellationToken.None)
                .GetAwaiter()
                .GetResult();
            _transport.Send(response);
        }
        catch (Exception exception)
        {
            _transport.Send(ProtocolErrorEncoder.Encode(message.SourceInstanceId, exception));
        }
    }

}

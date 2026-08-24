using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Transport;

internal interface IProtocolTransport
{
    void Send(ProtocolOutput message);
}

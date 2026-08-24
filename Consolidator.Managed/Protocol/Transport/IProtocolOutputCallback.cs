using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Transport;

internal interface IProtocolOutputCallback
{
    void Send(ProtocolOutput message);
}

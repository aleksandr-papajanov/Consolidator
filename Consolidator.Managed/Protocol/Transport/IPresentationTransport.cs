using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Transport;

internal interface IPresentationTransport
{
    void Send(ProtocolOutput message);

    void SetActive(ulong instanceId, bool active);

    void Unregister(ulong instanceId);
}

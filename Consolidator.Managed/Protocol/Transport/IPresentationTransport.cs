using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Transport;

internal interface IPresentationTransport
{
    void Send(ProtocolOutput message);

    void SetActive(ulong instanceId, bool active);

    void SetObservedTarget(ulong instanceId, ulong targetInstanceId, int bankId);

    void Unregister(ulong instanceId);
}
namespace Consolidator.Managed.Protocol.Transport;

internal interface IProtocolOutputRegistry
{
    void Register(
        ulong instanceId,
        IProtocolOutputCallback callback);

    void Unregister(ulong instanceId);

    IReadOnlyList<ulong> GetRegisteredInstanceIds();
}

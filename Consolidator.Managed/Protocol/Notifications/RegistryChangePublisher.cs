using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class RegistryChangePublisher
{
    private readonly IProtocolTransport _transport;
    private readonly IProtocolOutputRegistry _outputs;
    private long _revision;

    public RegistryChangePublisher(
        IProtocolTransport transport,
        IProtocolOutputRegistry outputs)
    {
        _transport = transport;
        _outputs = outputs;
    }

    public ulong Revision => (ulong)Interlocked.Read(ref _revision);

    public void Publish()
    {
        var revision = Interlocked.Increment(ref _revision);
        _transport.Send(new ProtocolOutput(
            _outputs.GetRegisteredInstanceIds(),
            "registry_changed",
            [
                new Atom(AtomType.Integer, 1, 0, null),
                new Atom(AtomType.Integer, revision, 0, null)
            ]));
    }
}

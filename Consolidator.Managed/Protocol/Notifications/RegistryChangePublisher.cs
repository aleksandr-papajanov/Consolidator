using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class RegistryChangePublisher
{
    private readonly IProtocolTransport _transport;
    private readonly IProtocolOutputRegistry _outputs;
    private readonly System.Collections.Concurrent.ConcurrentDictionary<ulong, byte> _observers = new();
    private long _revision;

    public RegistryChangePublisher(
        IProtocolTransport transport,
        IProtocolOutputRegistry outputs)
    {
        _transport = transport;
        _outputs = outputs;
    }

    public ulong Revision => (ulong)Interlocked.Read(ref _revision);

    public void RegisterObserver(ulong instanceId)
    {
        _observers[instanceId] = 1;
    }

    public void UnregisterObserver(ulong instanceId)
    {
        _observers.TryRemove(instanceId, out _);
    }

    public IReadOnlyList<ulong> GetObserverIds() =>
        _observers.Keys.OrderBy(instanceId => instanceId).ToArray();

    public void InstanceAdded(
        ulong instanceId,
        string label,
        IReadOnlyList<(int BankId, uint? GroupId)> banks)
    {
        var payload = new List<Atom>(3 + banks.Count * 2)
        {
            Symbol(instanceId.ToString()),
            Symbol(label),
            Integer(banks.Count)
        };
        foreach (var bank in banks)
        {
            payload.Add(Integer(bank.BankId));
            payload.Add(bank.GroupId is { } group ? Integer(group) : Symbol("none"));
        }
        Publish("registry_instance_added", payload.ToArray());
    }

    public void InstanceRemoved(ulong instanceId) =>
        Publish("registry_instance_removed", Symbol(instanceId.ToString()));

    public void LabelChanged(ulong instanceId, string label) =>
        Publish("registry_label_changed", Symbol(instanceId.ToString()), Symbol(label));

    public void BankGroupChanged(ulong instanceId, int bankId, uint? groupId) =>
        Publish(
            "registry_bank_group_changed",
            Symbol(instanceId.ToString()),
            Integer(bankId),
            groupId is { } group ? Integer(group) : Symbol("none"));

    public void Publish(string selector, params Atom[] payload)
    {
        var previousRevision = (ulong)Interlocked.Read(ref _revision);
        var revision = (ulong)Interlocked.Increment(ref _revision);
        var atoms = new Atom[3 + payload.Length];
        atoms[0] = new Atom(AtomType.Integer, 1, 0, null);
        atoms[1] = new Atom(AtomType.Integer, (long)previousRevision, 0, null);
        atoms[2] = new Atom(AtomType.Integer, (long)revision, 0, null);
        Array.Copy(payload, 0, atoms, 3, payload.Length);
        RuntimeMetrics.Shared.RecordRegistryDelta();
        _transport.Send(new ProtocolOutput(
            _outputs.GetRegisteredInstanceIds()
                .Where(instanceId => _observers.ContainsKey(instanceId))
                .ToArray(),
            selector,
            atoms));
    }

    private static Atom Integer(long value) => new(AtomType.Integer, value, 0, null);
    private static Atom Symbol(string value) => new(AtomType.Symbol, 0, 0, value);
}

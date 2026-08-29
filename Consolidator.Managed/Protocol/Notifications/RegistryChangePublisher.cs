using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class RegistryChangePublisher : IBankEffectStatusSink, IProcessorStatusSink
{
    private readonly IPresentationTransport _transport;
    private readonly IProtocolOutputRegistry _outputs;
    private readonly System.Collections.Concurrent.ConcurrentDictionary<ulong, byte> _observers = new();
    private long _revision;

    public RegistryChangePublisher(
        IPresentationTransport transport,
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
        bool mute,
        bool solo,
        IReadOnlyList<ProcessorStatus> processors,
        IReadOnlyList<(int BankId, uint? GroupId, bool EffectActive)> banks)
    {
        var payload = new List<Atom>(5 + banks.Count * 3)
        {
            Symbol(instanceId.ToString()),
            Symbol(label),
            Integer(mute ? 1 : 0),
            Integer(solo ? 1 : 0),
            Integer(processors.Count)
        };
        foreach (var processor in processors)
        {
            payload.Add(Symbol(ProcessorIds.Encode(processor.ProcessorId)));
            payload.Add(Integer(processor.EffectActive ? 1 : 0));
            payload.Add(Integer(processor.Bypassed ? 1 : 0));
            payload.Add(Integer(processor.Soloed ? 1 : 0));
        }
        payload.Add(Integer(banks.Count));
        foreach (var bank in banks)
        {
            payload.Add(Integer(bank.BankId));
            payload.Add(bank.GroupId is { } group ? Integer(group) : Symbol("none"));
            payload.Add(Integer(bank.EffectActive ? 1 : 0));
        }
        Publish("registry_instance_added", payload.ToArray());
    }

    public void InstanceRemoved(ulong instanceId) =>
        Publish("registry_instance_removed", Symbol(instanceId.ToString()));

    public void LabelChanged(ulong instanceId, string label) =>
        Publish("registry_label_changed", Symbol(instanceId.ToString()), Symbol(label));

    public void InstanceMuteChanged(ulong instanceId, bool mute) =>
        Publish("registry_instance_mute_changed",
            Symbol(instanceId.ToString()), Integer(mute ? 1 : 0));

    public void InstanceSoloChanged(ulong instanceId, bool solo) =>
        Publish("registry_instance_solo_changed",
            Symbol(instanceId.ToString()), Integer(solo ? 1 : 0));

    public void BankGroupChanged(ulong instanceId, int bankId, uint? groupId) =>
        Publish(
            "registry_bank_group_changed",
            Symbol(instanceId.ToString()),
            Integer(bankId),
            groupId is { } group ? Integer(group) : Symbol("none"));

    public void BankEffectStatusChanged(
        Consolidator.Managed.Core.State.InstanceId instanceId,
        int bankId,
        bool effectActive) =>
        Publish(
            "registry_bank_effect_changed",
            Symbol(instanceId.Value.ToString()),
            Integer(bankId),
            Integer(effectActive ? 1 : 0));

    public void ProcessorStatusChanged(InstanceId instanceId, ProcessorStatus status) =>
        Publish(
            "registry_processor_changed",
            Symbol(instanceId.Value.ToString()),
            Symbol(ProcessorIds.Encode(status.ProcessorId)),
            Integer(status.EffectActive ? 1 : 0),
            Integer(status.Bypassed ? 1 : 0),
            Integer(status.Soloed ? 1 : 0));

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
            atoms,
            DeliverySemantics.Lossless));
    }

    private static Atom Integer(long value) => new(AtomType.Integer, value, 0, null);
    private static Atom Symbol(string value) => new(AtomType.Symbol, 0, 0, value);
}

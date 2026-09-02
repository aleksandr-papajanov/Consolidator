namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IRegistryChangeSink
{
    ulong Revision { get; }

    event Action<string>? RegistryChangedEvent;

    event Action<ulong>? ObserverUnregisteredEvent;

    IReadOnlyList<ulong> GetObserverIds();

    void RegisterObserver(ulong instanceId);

    void UnregisterObserver(ulong instanceId);

    void InstanceAdded(
        ulong instanceId,
        string label,
        bool mute,
        bool solo,
        bool bypass,
        IReadOnlyList<ProcessorStatus> processors,
        IReadOnlyList<(int BankId, uint? GroupId, bool EffectActive)> banks);

    void InstanceRemoved(ulong instanceId);

    void LabelChanged(ulong instanceId, string label);

    void InstanceMuteChanged(ulong instanceId, bool mute);

    void InstanceSoloChanged(ulong instanceId, bool solo);

    void InstanceBypassChanged(ulong instanceId, bool bypass);

    void BankGroupChanged(ulong instanceId, int bankId, uint? groupId);

    void BankActivityChanged(InstanceId instanceId, int bankId, bool effectActive);

    void ProcessorActivityChanged(InstanceId instanceId, ProcessorStatus status);
}
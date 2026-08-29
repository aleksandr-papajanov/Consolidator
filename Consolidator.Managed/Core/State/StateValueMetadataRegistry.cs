using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State;

internal sealed class StateValueMetadataRegistry
{
    private readonly Dictionary<StateValueAddress, StateValueMetadata> _metadata = new();
    private readonly object _lock = new();

    public IStateValueObserver<TValue> Observe<TValue>(
        InstanceId instanceId,
        StatePath path,
        FloatRange? physicalRange,
        Func<BankAddress?, FloatRange?> effectiveRange)
    {
        return new MetadataObserver<TValue>(
            this,
            new StateValueAddress(instanceId, path),
            physicalRange,
            effectiveRange);
    }

    public StateValueMetadata Get(InstanceId instanceId, StatePath path)
    {
        lock (_lock)
        {
            return _metadata.TryGetValue(
                new StateValueAddress(instanceId, path),
                out var metadata)
                ? metadata
                : new StateValueMetadata(null, null);
        }
    }

    private void Attach(
        StateValueAddress address,
        FloatRange? physicalRange,
        Func<BankAddress?, FloatRange?> effectiveRange)
    {
        lock (_lock)
        {
            _metadata.Add(
                address,
                new StateValueMetadata(physicalRange, effectiveRange));
        }
    }

    private void Detach(StateValueAddress address)
    {
        lock (_lock)
        {
            _metadata.Remove(address);
        }
    }

    private readonly record struct StateValueAddress(
        InstanceId InstanceId,
        StatePath Path);

    private sealed class MetadataObserver<TValue> : IStateValueObserver<TValue>
    {
        private readonly StateValueMetadataRegistry _owner;
        private readonly StateValueAddress _address;
        private readonly FloatRange? _physicalRange;
        private readonly Func<BankAddress?, FloatRange?> _effectiveRange;

        public MetadataObserver(
            StateValueMetadataRegistry owner,
            StateValueAddress address,
            FloatRange? physicalRange,
            Func<BankAddress?, FloatRange?> effectiveRange)
        {
            _owner = owner;
            _address = address;
            _physicalRange = physicalRange;
            _effectiveRange = effectiveRange;
        }

        public void Attach(StateValue<TValue> value)
        {
            _owner.Attach(_address, _physicalRange, _effectiveRange);
        }

        public void ValueChanged(
            StateValue<TValue> value,
            TValue previousValue,
            TValue currentValue)
        {
        }

        public void Detach(StateValue<TValue> value)
        {
            _owner.Detach(_address);
        }
    }
}

internal sealed class StateValueMetadata
{
    private readonly Func<BankAddress?, FloatRange?>? _effectiveRange;

    public StateValueMetadata(
        FloatRange? physicalRange,
        Func<BankAddress?, FloatRange?>? effectiveRange)
    {
        PhysicalRange = physicalRange;
        _effectiveRange = effectiveRange;
    }

    public FloatRange? PhysicalRange { get; }

    public FloatRange? GetEffectiveRange(BankAddress? focusedBank) =>
        _effectiveRange?.Invoke(focusedBank);
}

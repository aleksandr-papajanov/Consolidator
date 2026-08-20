using Consolidator.Managed.Core.State.Bindings;

namespace Consolidator.Managed.Core.State;

public sealed class StateValue<TValue> : IStateValue, IHistoryValue
{
    private readonly StateHistory _history;
    private readonly TValue[] _values;
    private readonly IStateBinding<TValue>? _binding;
    private bool _disposed;

    internal StateValue(
        StateId id,
        StateHistory history,
        TValue initialValue,
        IStateBinding<TValue>? binding)
    {
        Id = id;
        _history = history;
        _values = new TValue[StateHistory.Capacity];
        _binding = binding;
        Array.Fill(_values, initialValue);
    }

    public StateId Id { get; }

    public TValue Value
    {
        get
        {
            return _history.GetValue(this);
        }
        set
        {
            _history.SetValue(this, value);
        }
    }

    void IHistoryValue.CopySlot(
        int sourceSlot,
        int destinationSlot)
    {
        _values[destinationSlot] = _values[sourceSlot];
    }

    void IHistoryValue.ApplySlot(int slot)
    {
        if (!_disposed)
        {
            _binding?.Apply(_values[slot]);
        }
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        _history.Remove(this);
    }

    internal TValue GetValue(int slot)
    {
        return _values[slot];
    }

    internal void SetValue(
        int slot,
        TValue value)
    {
        ObjectDisposedException.ThrowIf(
            _disposed,
            this);

        _values[slot] = value;
    }

    internal void ApplyValue(TValue value)
    {
        if (_disposed)
        {
            return;
        }

        _binding?.Apply(value);
    }
}

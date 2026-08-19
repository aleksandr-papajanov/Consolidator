namespace Consolidator.Managed.Core.State;

public sealed class StateValue<TValue> : IStateValue, IHistoryValue
{
    private readonly StateHistory _history;
    private readonly TValue[] _values;
    private readonly Action<TValue>? _apply;

    internal StateValue(
        StateId id,
        StateHistory history,
        TValue initialValue,
        Action<TValue>? apply)
    {
        Id = id;
        _history = history;
        _values = new TValue[StateHistory.Capacity];
        _apply = apply;
        Array.Fill(_values, initialValue);
    }

    public StateId Id { get; }

    public TValue Value
    {
        get
        {
            return _values[_history.CurrentSlot];
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
        _apply?.Invoke(_values[slot]);
    }

    public void Dispose()
    {
        _history.Remove(this);
    }

    internal void SetValue(TValue value)
    {
        _values[_history.CurrentSlot] = value;
        _apply?.Invoke(value);
    }
}

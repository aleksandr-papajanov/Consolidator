using Consolidator.Managed.Core.State.Bindings;

namespace Consolidator.Managed.Core.State;

public sealed class StateHistory
{
    public const int Capacity = 16;

    private readonly object _lock = new();
    private readonly List<IHistoryValue> _values = new();
    private int _currentSlot;
    private int _availableUndoCount;
    private int _availableRedoCount;

    public int CurrentSlot
    {
        get
        {
            lock (_lock)
            {
                return _currentSlot;
            }
        }
    }

    public bool CanUndo
    {
        get
        {
            lock (_lock)
            {
                return _availableUndoCount > 0;
            }
        }
    }

    public bool CanRedo
    {
        get
        {
            lock (_lock)
            {
                return _availableRedoCount > 0;
            }
        }
    }

    /// <summary>
    /// Registers a value in the shared history.
    /// </summary>
    /// <remarks>
    /// The initial binding is applied during registration. Bindings for later
    /// history operations run after the history lock is released. They must
    /// not start another history operation, perform I/O, or do long-running
    /// work.
    /// </remarks>
    public StateValue<TValue> CreateValue<TValue>(
        StateId id,
        TValue initialValue,
        IStateBinding<TValue>? binding = null)
    {
        lock (_lock)
        {
            var value = new StateValue<TValue>(
                id,
                this,
                initialValue,
                binding);
            _values.Add(value);

            try
            {
                binding?.Apply(initialValue);
            }
            catch
            {
                _values.Remove(value);
                throw;
            }

            return value;
        }
    }

    /// <summary>
    /// Opens the next history point before a logical operation writes values.
    /// </summary>
    public void AdvanceHistoryPoint()
    {
        lock (_lock)
        {
            var destinationSlot = NextSlot(_currentSlot);

            foreach (var value in _values)
            {
                value.CopySlot(_currentSlot, destinationSlot);
            }

            _currentSlot = destinationSlot;
            _availableUndoCount = Math.Min(
                _availableUndoCount + 1,
                Capacity - 1);
            _availableRedoCount = 0;
        }
    }

    public bool Undo()
    {
        IHistoryValue[] values;
        int currentSlot;

        lock (_lock)
        {
            if (_availableUndoCount == 0)
            {
                return false;
            }

            _currentSlot = PreviousSlot(_currentSlot);
            _availableUndoCount--;
            _availableRedoCount++;
            currentSlot = _currentSlot;
            values = _values.ToArray();
        }

        ApplyCurrent(values, currentSlot);
        return true;
    }

    public bool Redo()
    {
        IHistoryValue[] values;
        int currentSlot;

        lock (_lock)
        {
            if (_availableRedoCount == 0)
            {
                return false;
            }

            _currentSlot = NextSlot(_currentSlot);
            _availableUndoCount++;
            _availableRedoCount--;
            currentSlot = _currentSlot;
            values = _values.ToArray();
        }

        ApplyCurrent(values, currentSlot);
        return true;
    }

    internal void SetValue<TValue>(
        StateValue<TValue> value,
        TValue newValue)
    {
        lock (_lock)
        {
            value.SetValue(
                _currentSlot,
                newValue);
        }

        value.ApplyValue(newValue);
    }

    internal TValue GetValue<TValue>(StateValue<TValue> value)
    {
        lock (_lock)
        {
            return value.GetValue(_currentSlot);
        }
    }

    internal void Remove(IHistoryValue value)
    {
        lock (_lock)
        {
            _values.Remove(value);
        }
    }

    private static void ApplyCurrent(
        IReadOnlyList<IHistoryValue> values,
        int slot)
    {
        foreach (var value in values)
        {
            value.ApplySlot(slot);
        }
    }

    private static int NextSlot(int slot)
    {
        return (slot + 1) % Capacity;
    }

    private static int PreviousSlot(int slot)
    {
        return (slot + Capacity - 1) % Capacity;
    }
}

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
    /// The binding runs while the history lock is held. It must only
    /// perform fast local Managed projection updates. It must not publish DSP,
    /// send output, call Coordinator or history operations, perform I/O, or do
    /// long-running work.
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
        lock (_lock)
        {
            if (_availableUndoCount == 0)
            {
                return false;
            }

            _currentSlot = PreviousSlot(_currentSlot);
            _availableUndoCount--;
            _availableRedoCount++;
            ApplyCurrent();
            return true;
        }
    }

    public bool Redo()
    {
        lock (_lock)
        {
            if (_availableRedoCount == 0)
            {
                return false;
            }

            _currentSlot = NextSlot(_currentSlot);
            _availableUndoCount++;
            _availableRedoCount--;
            ApplyCurrent();
            return true;
        }
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

    private void ApplyCurrent()
    {
        foreach (var value in _values)
        {
            value.ApplySlot(_currentSlot);
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

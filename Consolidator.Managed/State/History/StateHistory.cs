namespace Consolidator.Managed.State.History;

public sealed class StateHistory
{
    public const int Capacity = 16;

    private readonly object _lock = new();
    private readonly LinkedList<IHistoryValue> _values = new();
    private readonly Dictionary<IHistoryValue, LinkedListNode<IHistoryValue>>
        _valueNodes = new();
    private int _currentSlot;
    private int _availableUndoCount;
    private int _availableRedoCount;
    private ulong _revision;

    public event Action<StateHistorySnapshot>? Changed;

    internal void Register(IHistoryValue value)
    {
        ArgumentNullException.ThrowIfNull(value);

        lock (_lock)
        {
            if (!_valueNodes.ContainsKey(value))
            {
                var node = _values.AddLast(value);
                _valueNodes.Add(value, node);
                value.SetCurrentSlot(_currentSlot);
            }
        }
    }

    internal void Unregister(IHistoryValue value)
    {
        ArgumentNullException.ThrowIfNull(value);

        lock (_lock)
        {
            if (_valueNodes.Remove(value, out var node))
            {
                _values.Remove(node);
            }
        }
    }

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

    public StateHistorySnapshot Snapshot
    {
        get
        {
            lock (_lock)
            {
                return CreateSnapshot();
            }
        }
    }

    public StateHistoryTransaction BeginTransaction()
    {
        return new StateHistoryTransaction();
    }

    public void AdvanceHistoryPoint()
    {
        StateHistorySnapshot snapshot;
        lock (_lock)
        {
            var destinationSlot = NextSlot(_currentSlot);

            var values = GetHistoryValues();
            foreach (var value in values)
            {
                value.CopySlot(_currentSlot, destinationSlot);
            }
            _currentSlot = destinationSlot;
            SetCurrentSlot(values, _currentSlot);
            _availableUndoCount = Math.Min(
                _availableUndoCount + 1,
                Capacity - 1);
            _availableRedoCount = 0;
            snapshot = CreateSnapshot(++_revision);
        }

        Changed?.Invoke(snapshot);
    }

    public bool JumpToHistory(int cursor)
    {
        IHistoryValue[] values;
        int targetSlot;
        StateHistorySnapshot snapshot;

        lock (_lock)
        {
            var entryCount = _availableUndoCount + _availableRedoCount;
            if (cursor < 0 || cursor > entryCount)
            {
                return false;
            }

            _currentSlot = MoveSlot(_currentSlot, cursor - _availableUndoCount);
            _availableUndoCount = cursor;
            _availableRedoCount = entryCount - cursor;
            targetSlot = _currentSlot;
            values = GetHistoryValues();
            SetCurrentSlot(values, _currentSlot);
            snapshot = CreateSnapshot(++_revision);
        }

        ApplyCurrent(values, targetSlot);
        Changed?.Invoke(snapshot);
        return true;
    }

    private static void ApplyCurrent(IReadOnlyList<IHistoryValue> values, int slot)
    {
        foreach (var value in values)
        {
            value.ApplySlot(slot);
        }
    }

    private IHistoryValue[] GetHistoryValues()
    {
        return _values.ToArray();
    }

    private static void SetCurrentSlot(
        IReadOnlyList<IHistoryValue> values,
        int slot)
    {
        foreach (var value in values)
        {
            value.SetCurrentSlot(slot);
        }
    }

    private static int NextSlot(int slot) => (slot + 1) % Capacity;

    private static int MoveSlot(int slot, int offset)
    {
        var step = offset >= 0 ? 1 : -1;
        for (var index = 0; index < Math.Abs(offset); index++)
        {
            slot = step > 0 ? NextSlot(slot) : PreviousSlot(slot);
        }

        return slot;
    }

    private StateHistorySnapshot CreateSnapshot(ulong? revision = null)
    {
        return new StateHistorySnapshot(
            revision ?? _revision,
            _availableUndoCount,
            _availableUndoCount + _availableRedoCount,
            _availableUndoCount > 0,
            _availableRedoCount > 0);
    }

    private static int PreviousSlot(int slot) =>
        (slot + Capacity - 1) % Capacity;
}

public sealed record StateHistorySnapshot(
    ulong Revision,
    int Cursor,
    int EntryCount,
    bool CanUndo,
    bool CanRedo);





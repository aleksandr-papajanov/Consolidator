using Consolidator.Managed.State.History;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.State;

public sealed class StateValue<TValue> : IHistoryValue, IDisposable
{
    private readonly TValue[] _values;
    private readonly TValue _initialValue;
    private readonly List<IStateValueObserver<TValue>> _observers;
    private readonly Action<IHistoryValue> _unregisterHistory;
    private TValue _pendingValue = default!;
    private TValue _previousValue = default!;
    private StateHistoryTransaction? _pendingTransaction;
    private TValue[]? _pendingBaselineValues;
    private int _pendingBaselineSlot;
    private Action<TValue>? _mutationHandler;
    private Action<TValue, StateValueEditMode, StateHistoryTransaction>? _mutationPreparationHandler;
    private Func<StateHistoryTransaction, int>? _resetPreparationHandler;
    private TValue _previousSlotValue = default!;
    private bool _disposed;
    private int _currentSlot;

    internal StateValue(
        TValue initialValue,
        IReadOnlyList<IStateValueObserver<TValue>> observers,
        Action<IHistoryValue> unregisterHistory)
    {
        _values = new TValue[StateHistory.Capacity];
        _initialValue = initialValue;
        _observers = observers.ToList();
        _unregisterHistory = unregisterHistory;
        Array.Fill(_values, initialValue);

        var attachedCount = 0;
        try
        {
            foreach (var observer in _observers)
            {
                observer.Attach(this);
                attachedCount++;
            }
        }
        catch
        {
            for (var index = attachedCount - 1; index >= 0; index--)
            {
                _observers[index].Detach(this);
            }

            throw;
        }
    }

    public TValue Value
    {
        get
        {
            return _values[_currentSlot];
        }
        set
        {
            ObjectDisposedException.ThrowIf(_disposed, this);
            if (_mutationHandler is not null)
            {
                _mutationHandler(value);
                return;
            }

            SetDirect(value);
        }
    }

    internal bool PrepareReset(StateHistoryTransaction transaction)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        ArgumentNullException.ThrowIfNull(transaction);
        if (_resetPreparationHandler is not null)
        {
            return _resetPreparationHandler(transaction) > 0;
        }

        if (EqualityComparer<TValue>.Default.Equals(Value, _initialValue))
        {
            return false;
        }

        PrepareMutation(_initialValue, StateValueEditMode.CopyValue, transaction);
        return true;
    }

    void IHistoryValue.CopySlot(
        int sourceSlot,
        int destinationSlot)
    {
        _values[destinationSlot] = _values[sourceSlot];
    }

    void IHistoryValue.SetCurrentSlot(int slot)
    {
        _previousSlotValue = _values[_currentSlot];
        _currentSlot = slot;
    }

    void IHistoryValue.ApplySlot(int slot)
    {
        if (!_disposed)
        {
            if (!EqualityComparer<TValue>.Default.Equals(_previousSlotValue, _values[slot]))
            {
                NotifyObservers(_previousSlotValue, _values[slot]);
            }
        }
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        foreach (var observer in _observers)
        {
            observer.Detach(this);
        }

        _unregisterHistory(this);
    }

    internal void Prepare(
        TValue value,
        StateHistoryTransaction transaction)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        ArgumentNullException.ThrowIfNull(transaction);

        if (_pendingTransaction is not null &&
            !ReferenceEquals(_pendingTransaction, transaction))
        {
            throw new InvalidOperationException(
                "The state value already belongs to another transaction.");
        }
        if (ReferenceEquals(_pendingTransaction, transaction))
        {
            throw new InvalidOperationException(
                "The state value was prepared more than once in one transaction.");
        }

        var previousValue = Value;
        var pendingValue = value;
        if (EqualityComparer<TValue>.Default.Equals(previousValue, pendingValue))
        {
            return;
        }

        _previousValue = previousValue;
        _pendingValue = pendingValue;
        _pendingTransaction = transaction;
        transaction.AddCommittedChange(() =>
            NotifyObservers(previousValue, pendingValue));
        transaction.Add(new StateValueTransactionEntry(this));
    }

    internal void PrepareMutation(
        TValue value,
        StateValueEditMode editMode,
        StateHistoryTransaction transaction)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        ArgumentNullException.ThrowIfNull(transaction);
        if (_mutationPreparationHandler is not null)
        {
            _mutationPreparationHandler(value, editMode, transaction);
            return;
        }

        Prepare(value, transaction);
    }

    internal void PrepareBaseline(TValue value, StateHistoryTransaction transaction)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        ArgumentNullException.ThrowIfNull(transaction);
        if (_pendingTransaction is not null)
        {
            throw new InvalidOperationException(
                "The state value already belongs to a transaction.");
        }

        var previousValue = Value;
        var comparer = EqualityComparer<TValue>.Default;
        var baselineAlreadyApplied = true;
        foreach (var existingValue in _values)
        {
            if (!comparer.Equals(existingValue, value))
            {
                baselineAlreadyApplied = false;
                break;
            }
        }

        if (baselineAlreadyApplied)
        {
            return;
        }

        _pendingBaselineValues = (TValue[])_values.Clone();
        _pendingBaselineSlot = _currentSlot;
        _pendingTransaction = transaction;
        transaction.Add(new BaselineTransactionEntry(this, value));
        if (!comparer.Equals(previousValue, value))
        {
            transaction.AddCommittedChange(() =>
                NotifyObservers(previousValue, value));
        }
    }

    private void CommitBaseline(TValue value)
    {
        Array.Fill(_values, value);
        _previousSlotValue = value;
    }

    private void RollbackBaseline()
    {
        if (_pendingBaselineValues is null)
        {
            return;
        }
        Array.Copy(_pendingBaselineValues, _values, _values.Length);
        _currentSlot = _pendingBaselineSlot;
        _previousSlotValue = _values[_currentSlot];
        _pendingBaselineValues = null;
        _pendingTransaction = null;
    }

    internal void SetMutationHandler(
        Action<TValue> mutationHandler,
        Action<TValue, StateValueEditMode, StateHistoryTransaction> mutationPreparationHandler)
    {
        ArgumentNullException.ThrowIfNull(mutationHandler);
        ArgumentNullException.ThrowIfNull(mutationPreparationHandler);
        _mutationHandler = mutationHandler;
        _mutationPreparationHandler = mutationPreparationHandler;
    }

    internal void SetResetPreparationHandler(
        Func<StateHistoryTransaction, int> resetPreparationHandler)
    {
        ArgumentNullException.ThrowIfNull(resetPreparationHandler);
        _resetPreparationHandler = resetPreparationHandler;
    }

    internal bool PrepareResetDirect(StateHistoryTransaction transaction)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        ArgumentNullException.ThrowIfNull(transaction);
        if (EqualityComparer<TValue>.Default.Equals(Value, _initialValue))
        {
            return false;
        }

        Prepare(_initialValue, transaction);
        return true;
    }

    private void SetDirect(TValue value, bool notify = true)
    {
        var previousValue = _values[_currentSlot];
        _values[_currentSlot] = value;
        if (notify && !EqualityComparer<TValue>.Default.Equals(previousValue, value))
        {
            NotifyObservers(previousValue, value);
        }
    }

    private void CommitPending()
    {
        if (_pendingTransaction is null)
        {
            return;
        }

        SetDirect(_pendingValue, false);
    }

    private void RollbackPending()
    {
        if (_pendingTransaction is null)
        {
            return;
        }

        _values[_currentSlot] = _previousValue;
        _pendingTransaction = null;
    }

    private sealed class StateValueTransactionEntry : IStateTransactionEntry
    {
        private readonly StateValue<TValue> _value;

        public StateValueTransactionEntry(StateValue<TValue> value)
        {
            _value = value;
        }

        public void Commit()
        {
            _value.CommitPending();
        }

        public void Rollback()
        {
            _value.RollbackPending();
        }

        public void Complete()
        {
            _value._pendingTransaction = null;
        }
    }

    private sealed class BaselineTransactionEntry : IStateTransactionEntry
    {
        private readonly StateValue<TValue> _value;
        private readonly TValue _pendingValue;

        public BaselineTransactionEntry(StateValue<TValue> value, TValue pendingValue)
        {
            _value = value;
            _pendingValue = pendingValue;
        }

        public void Commit()
        {
            _value.CommitBaseline(_pendingValue);
        }

        public void Rollback()
        {
            _value.RollbackBaseline();
        }

        public void Complete()
        {
            _value._pendingBaselineValues = null;
            _value._pendingTransaction = null;
        }
    }

    private void NotifyObservers(
        TValue previousValue,
        TValue currentValue)
    {
        if (_disposed)
        {
            return;
        }

        foreach (var observer in _observers)
        {
            observer.ValueChanged(this, previousValue, currentValue);
        }
    }
}





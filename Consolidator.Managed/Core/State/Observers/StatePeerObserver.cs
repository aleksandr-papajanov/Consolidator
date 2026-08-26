using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.Topology;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;
using Consolidator.Managed.State.Observers;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed class StatePeerObserver
{
    private readonly StateHistory _history;
    private readonly TopologyIndex _topology;
    private readonly object _lock = new();
    private readonly Dictionary<InstanceId, List<IObservedValue>> _valuesByInstance = new();
    private readonly Dictionary<ObservedValueAddress, IObservedValue> _valuesByAddress = new();
    private readonly Dictionary<BankValueAddress, IObservedValue> _bankValuesByAddress = new();
    private BankAddress? _editingBank;
    private bool _isEditing;

    public StatePeerObserver(
        StateHistory history,
        TopologyIndex topology)
    {
        _history = history;
        _topology = topology;
    }

    public IStatePeerValueObserver<TValue> Create<TValue>(
        InstanceId instanceId,
        StatePath path,
        StateValueEditScope scope,
        StateValueEditMode editMode,
        FloatRange? physicalRange,
        Action<TValue> effectiveRangeChanged)
    {
        ArgumentNullException.ThrowIfNull(effectiveRangeChanged);
        Validate<TValue>(editMode, physicalRange);
        return new StatePeerValueObserver<TValue>(
            this,
            instanceId,
            path,
            scope,
            editMode,
            physicalRange,
            effectiveRangeChanged);
    }

    public void Refresh(InstanceId changedInstanceId)
    {
        Refresh([changedInstanceId]);
    }

    public void Refresh(IReadOnlyList<InstanceId> changedInstanceIds)
    {
        ArgumentNullException.ThrowIfNull(changedInstanceIds);

        var affectedInstanceIds = changedInstanceIds.ToHashSet();
        InstanceId[] registeredInstanceIds;
        lock (_lock)
        {
            registeredInstanceIds = _valuesByInstance.Keys.ToArray();
        }

        foreach (var changedInstanceId in changedInstanceIds)
        {
            affectedInstanceIds.UnionWith(
                _topology.ResolveStatePeerInstanceIds(changedInstanceId));
        }
        foreach (var instanceId in registeredInstanceIds)
        {
            if (changedInstanceIds.Any(changedInstanceId =>
                _topology.ResolveStatePeerInstanceIds(instanceId)
                    .Contains(changedInstanceId)))
            {
                affectedInstanceIds.Add(instanceId);
            }
        }

        IObservedValue[] values;
        lock (_lock)
        {
            values = affectedInstanceIds
                .OrderBy(instanceId => instanceId.Value)
                .SelectMany(instanceId =>
                    _valuesByInstance.TryGetValue(instanceId, out var entries)
                        ? entries.AsEnumerable()
                        : Enumerable.Empty<IObservedValue>())
                .ToArray();
        }

        var assigned = new HashSet<IObservedValue>();
        foreach (var value in values)
        {
            if (!affectedInstanceIds.Contains(value.InstanceId) ||
                !assigned.Add(value))
            {
                continue;
            }

            var sharesPeerSet = value.Scope is StateValueEditScope.Local ||
                value.Bank is not null;
            var peerSet = new PeerSet(
                ResolvePeers(value),
                sharesPeerSet);
            if (!sharesPeerSet)
            {
                value.SetPeers(peerSet);
                peerSet.UpdateEffectiveRanges(value, true);
                continue;
            }

            foreach (var peer in peerSet.Values)
            {
                peer.SetPeers(peerSet);
                assigned.Add(peer);
            }
            peerSet.UpdateEffectiveRanges(value, true);
        }
    }

    public void BeginEdit(BankAddress? focusedBank)
    {
        if (_isEditing)
        {
            throw new InvalidOperationException(
                "A state edit context is already active.");
        }

        _editingBank = focusedBank;
        _isEditing = true;
    }

    public void EndEdit()
    {
        _editingBank = null;
        _isEditing = false;
    }

    private void Attach(IObservedValue value)
    {
        lock (_lock)
        {
            if (!_valuesByInstance.TryGetValue(
                    value.InstanceId,
                    out var instanceValues))
            {
                instanceValues = new List<IObservedValue>();
                _valuesByInstance.Add(value.InstanceId, instanceValues);
            }
            instanceValues.Add(value);
            _valuesByAddress.Add(
                new ObservedValueAddress(value.InstanceId, value.Path),
                value);
            if (value.Bank is { } bank)
            {
                _bankValuesByAddress.Add(
                    new BankValueAddress(
                        bank,
                        value.PeerPath,
                        value.ValueType),
                    value);
            }
        }
    }

    private void Detach(IObservedValue value)
    {
        lock (_lock)
        {
            if (_valuesByInstance.TryGetValue(
                    value.InstanceId,
                    out var instanceValues))
            {
                instanceValues.Remove(value);
                if (instanceValues.Count == 0)
                {
                    _valuesByInstance.Remove(value.InstanceId);
                }
            }
            _valuesByAddress.Remove(
                new ObservedValueAddress(value.InstanceId, value.Path));
            if (value.Bank is { } bank)
            {
                _bankValuesByAddress.Remove(
                    new BankValueAddress(
                        bank,
                        value.PeerPath,
                        value.ValueType));
            }
        }
    }

    private void ValueChanged(IObservedValue changedValue)
    {
        if (changedValue.Peers.HasPendingEffectiveRangeRefresh)
        {
            return;
        }

        changedValue.Peers.UpdateEffectiveRanges(changedValue, false);
    }

    private IReadOnlyList<IObservedValue> ResolvePeers(IObservedValue value)
    {
        if (value.Scope is StateValueEditScope.Local)
        {
            return [value];
        }

        if (value.Bank is { } bank)
        {
            var connectedBanks = _topology.GetConnectedBankPeers(bank);
            lock (_lock)
            {
                return connectedBanks
                    .Select(candidateBank =>
                        _bankValuesByAddress.TryGetValue(
                            new BankValueAddress(
                                candidateBank,
                                value.PeerPath,
                                value.ValueType),
                            out var candidate)
                                ? candidate
                                : null)
                    .Where(candidate => candidate is not null)
                    .Cast<IObservedValue>()
                    .ToArray();
            }
        }

        return ResolveInstancePeers(
            value,
            _topology.ResolveStatePeerInstanceIds(value.InstanceId));
    }

    private PeerSet ResolveMutationPeers(IObservedValue value)
    {
        if (value.Scope is StateValueEditScope.Local ||
            value.Bank is not null ||
            !_isEditing)
        {
            return value.Peers;
        }

        return new PeerSet(
            ResolveFocusedPeers(value, _editingBank),
            false);
    }

    private PeerSet ResolvePresentationPeers(
        IObservedValue value,
        BankAddress? focusedBank)
    {
        if (value.Scope is StateValueEditScope.Local ||
            value.Bank is not null ||
            focusedBank is null)
        {
            return value.Peers;
        }

        return new PeerSet(
            ResolveFocusedPeers(value, focusedBank),
            false);
    }

    private IReadOnlyList<IObservedValue> ResolveFocusedPeers(
        IObservedValue value,
        BankAddress? focusedBank)
    {
        IReadOnlyList<InstanceId> targetIds = focusedBank is { } bank &&
            bank.InstanceId == value.InstanceId
                ? _topology.GetConnectedBankPeers(bank)
                    .Select(peer => peer.InstanceId)
                    .Append(value.InstanceId)
                    .Distinct()
                    .ToArray()
                : [value.InstanceId];
        return ResolveInstancePeers(value, targetIds);
    }

    private IReadOnlyList<IObservedValue> ResolveInstancePeers(
        IObservedValue value,
        IReadOnlyList<InstanceId> targetIds)
    {
        lock (_lock)
        {
            return targetIds
                .Select(instanceId => _valuesByAddress.TryGetValue(
                    new ObservedValueAddress(instanceId, value.Path),
                    out var candidate)
                        ? candidate
                        : null)
                .Where(candidate => candidate is not null &&
                    candidate.ValueType == value.ValueType)
                .Cast<IObservedValue>()
                .ToArray();
        }
    }

    private static void Validate<TValue>(
        StateValueEditMode editMode,
        FloatRange? physicalRange)
    {
        if (editMode is StateValueEditMode.ApplyDelta && typeof(TValue) != typeof(float))
        {
            throw new ArgumentException(
                "ApplyDelta requires a float state value.",
                nameof(editMode));
        }

        if (editMode is StateValueEditMode.ApplyDelta && physicalRange is null)
        {
            throw new ArgumentException(
                "ApplyDelta requires a physical range.",
                nameof(physicalRange));
        }

        if (editMode is StateValueEditMode.CopyValue && physicalRange is not null)
        {
            throw new ArgumentException(
                "CopyValue values must not define a physical range.",
                nameof(physicalRange));
        }
    }

    private interface IObservedValue
    {
        InstanceId InstanceId { get; }

        StatePath Path { get; }

        StatePath PeerPath { get; }

        BankAddress? Bank { get; }

        Type ValueType { get; }

        StateValueEditScope Scope { get; }

        PeerSet Peers { get; }

        bool TracksEffectiveRange { get; }

        object? GetCurrentValue();

        void SetPeers(PeerSet peers);

        FloatRange CalculateEffectiveDeltaRange();

        void SetEffectiveDeltaRange(FloatRange range, bool notify);
    }

    private sealed class StatePeerValueObserver<TValue> :
        IObservedValue,
        IStatePeerValueObserver<TValue>
    {
        private readonly StatePeerObserver _owner;
        private readonly StateValueEditMode _editMode;
        private readonly FloatRange? _physicalRange;
        private readonly Action<TValue> _effectiveRangeChanged;
        private StateValue<TValue>? _value;
        private PeerSet _peers = PeerSet.Empty;
        private FloatRange _effectiveDeltaRange = new(1.0F, 0.0F);

        public StatePeerValueObserver(
            StatePeerObserver owner,
            InstanceId instanceId,
            StatePath path,
            StateValueEditScope scope,
            StateValueEditMode editMode,
            FloatRange? physicalRange,
            Action<TValue> effectiveRangeChanged)
        {
            _owner = owner;
            InstanceId = instanceId;
            Path = path;
            Bank = owner._topology.ResolveBankAddress(instanceId, path);
            PeerPath = Bank is null
                ? path
                : new StatePath(path.Nodes.Select(node =>
                    IsBankNode(node) ? StateNodeIds.BankAt(0) : node));
            Scope = scope;
            _editMode = editMode;
            _physicalRange = physicalRange;
            _effectiveRangeChanged = effectiveRangeChanged;
        }

        public InstanceId InstanceId { get; }

        public StatePath Path { get; }

        public StatePath PeerPath { get; }

        public BankAddress? Bank { get; }

        public Type ValueType => typeof(TValue);

        public StateValueEditScope Scope { get; }

        public PeerSet Peers => _peers;

        public bool TracksEffectiveRange =>
            _editMode is StateValueEditMode.ApplyDelta;

        public FloatRange? GetEffectiveRange(BankAddress? focusedBank = null)
        {
            if (_editMode is not StateValueEditMode.ApplyDelta ||
                _value is null)
            {
                return null;
            }

            var deltaRange = focusedBank is null
                ? _effectiveDeltaRange
                : CalculateEffectiveDeltaRange(
                    _owner.ResolvePresentationPeers(this, focusedBank).Values);
            if (!deltaRange.IsValid)
            {
                return null;
            }

            var currentValue = (float)(object)_value.Value!;
            return new FloatRange(
                currentValue + deltaRange.Minimum,
                currentValue + deltaRange.Maximum);
        }

        public object? GetCurrentValue()
        {
            return _value is null ? null : _value.Value;
        }

        public void Attach(StateValue<TValue> value)
        {
            _value = value;
            value.SetMutationHandler(Set, Prepare);
            _owner.Attach(this);
        }

        public void ValueChanged(
            StateValue<TValue> value,
            TValue previousValue,
            TValue currentValue)
        {
            _owner.ValueChanged(this);
        }

        public void Detach(StateValue<TValue> value)
        {
            _owner.Detach(this);
            _value = null;
            _peers = PeerSet.Empty;
        }

        public void SetPeers(PeerSet peers)
        {
            ArgumentNullException.ThrowIfNull(peers);
            _peers = peers;
        }

        public FloatRange CalculateEffectiveDeltaRange() =>
            CalculateEffectiveDeltaRange(_peers.Values);

        private FloatRange CalculateEffectiveDeltaRange(
            IReadOnlyList<IObservedValue> peers)
        {
            if (_editMode is not StateValueEditMode.ApplyDelta)
            {
                return new FloatRange(1.0F, 0.0F);
            }

            var minimum = float.NegativeInfinity;
            var maximum = float.PositiveInfinity;
            foreach (var peer in peers.Cast<StatePeerValueObserver<TValue>>())
            {
                if (peer._physicalRange is not { } range || peer._value is null)
                {
                    throw new InvalidOperationException(
                        $"Peer {peer.InstanceId} has no physical range.");
                }

                var peerValue = (float)(object)peer._value.Value!;
                minimum = Math.Max(minimum, range.Minimum - peerValue);
                maximum = Math.Min(maximum, range.Maximum - peerValue);
            }

            return peers.Count == 0
                ? new FloatRange(1.0F, 0.0F)
                : new FloatRange(minimum, maximum);
        }

        public void SetEffectiveDeltaRange(FloatRange range, bool notify)
        {
            var changed = _effectiveDeltaRange != range;
            _effectiveDeltaRange = range;
            if (changed && notify && _value is not null)
            {
                _effectiveRangeChanged(_value.Value);
            }
        }

        private void Set(TValue value)
        {
            if (_peers.Values.Count == 0)
            {
                throw new InvalidOperationException(
                    $"No state peers are available for path {Path}.");
            }

            using var transaction = _owner._history.BeginTransaction();
            Prepare(value, transaction);
            transaction.Commit();
        }

        private void Prepare(
            TValue value,
            StateHistoryTransaction transaction)
        {
            ArgumentNullException.ThrowIfNull(transaction);
            if (_value is null)
            {
                throw new ObjectDisposedException(nameof(StateValue<TValue>));
            }

            var peers = _owner.ResolveMutationPeers(this);
            if (peers.Values.Count == 0)
            {
                throw new InvalidOperationException(
                    $"No state peers are available for path {Path}.");
            }

            var delta = _editMode is StateValueEditMode.ApplyDelta
                ? Subtract(value, _value.Value)
                : default;
            if (_editMode is StateValueEditMode.ApplyDelta &&
                !CalculateEffectiveDeltaRange(peers.Values).Contains(
                    (float)(object)delta!))
            {
                throw new InvalidOperationException(
                    $"The requested delta is outside the effective range for path {Path}.");
            }

                    peers.ScheduleEffectiveRangeRefresh(transaction, this);

            foreach (var peer in peers.Values.Cast<StatePeerValueObserver<TValue>>())
            {
                if (peer._value is null)
                {
                    throw new InvalidOperationException(
                        $"A state peer was removed for path {Path}.");
                }

                var peerValue = ReferenceEquals(peer, this)
                    ? value
                    : _editMode is StateValueEditMode.ApplyDelta
                        ? Add(peer._value.Value, delta!)
                        : value;
                peer._value.Prepare(peerValue, transaction);
            }
        }

        private static TValue Add(TValue left, TValue right) =>
            Calculate(left, right, (first, second) => first + second);

        private static TValue Subtract(TValue left, TValue right) =>
            Calculate(left, right, (first, second) => first - second);

        private static TValue Calculate(
            TValue left,
            TValue right,
            Func<float, float, float> operation)
        {
            if (typeof(TValue) != typeof(float))
            {
                throw new InvalidOperationException(
                    $"Delta editing is not supported for {typeof(TValue).Name}.");
            }

            return (TValue)(object)operation(
                (float)(object)left!,
                (float)(object)right!);
        }

        private static bool IsBankNode(NodeId node) =>
            node.Value >= 100 &&
            node.Value < 100 + DspConstants.BankCount;
    }

    private readonly record struct ObservedValueAddress(
        InstanceId InstanceId,
        StatePath Path);

    private readonly record struct BankValueAddress(
        BankAddress Bank,
        StatePath PeerPath,
        Type ValueType);

    private sealed class PeerSet
    {
        private object?[]? _effectiveRangeValues;
        private readonly bool _sharesEffectiveRange;
        private readonly bool _tracksEffectiveRange;
        private StateHistoryTransaction? _pendingEffectiveRangeRefresh;

        public PeerSet(
            IReadOnlyList<IObservedValue> values,
            bool sharesEffectiveRange)
        {
            ArgumentNullException.ThrowIfNull(values);
            Values = values;
            _sharesEffectiveRange = sharesEffectiveRange;
            _tracksEffectiveRange = values.Any(value =>
                value.TracksEffectiveRange);
        }

        public static PeerSet Empty { get; } =
            new(Array.Empty<IObservedValue>(), true);

        public IReadOnlyList<IObservedValue> Values { get; }

        public bool HasPendingEffectiveRangeRefresh =>
            _pendingEffectiveRangeRefresh is { IsCompleted: false };

        public void ScheduleEffectiveRangeRefresh(
            StateHistoryTransaction transaction,
            IObservedValue source)
        {
            if (!_tracksEffectiveRange ||
                ReferenceEquals(_pendingEffectiveRangeRefresh, transaction))
            {
                return;
            }

            _pendingEffectiveRangeRefresh = transaction;
            transaction.AddCommittedChange(() =>
            {
                _pendingEffectiveRangeRefresh = null;
                UpdateEffectiveRanges(source, true);
            });
        }

        public void UpdateEffectiveRanges(
            IObservedValue source,
            bool notify)
        {
            if (!_tracksEffectiveRange)
            {
                return;
            }

            if (_effectiveRangeValues is not null &&
                _effectiveRangeValues.Length == Values.Count)
            {
                var unchanged = true;
                for (var index = 0; index < Values.Count; index++)
                {
                    if (!Equals(
                        _effectiveRangeValues[index],
                        Values[index].GetCurrentValue()))
                    {
                        unchanged = false;
                        break;
                    }
                }
                if (unchanged)
                {
                    return;
                }
            }

            _effectiveRangeValues = Values
                .Select(value => value.GetCurrentValue())
                .ToArray();
            var effectiveRange = source.CalculateEffectiveDeltaRange();
            IReadOnlyList<IObservedValue> targets = _sharesEffectiveRange
                ? Values
                : [source];
            foreach (var value in targets)
            {
                value.SetEffectiveDeltaRange(effectiveRange, notify);
            }
        }
    }
}

internal interface IStatePeerValueObserver<TValue> : IStateValueObserver<TValue>
{
    FloatRange? GetEffectiveRange(BankAddress? focusedBank = null);
}

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
    private readonly Dictionary<ContextualPeerSetKey, PeerSet> _contextualPeerSets = new();
    private readonly Dictionary<IObservedValue, HashSet<PeerSet>> _peerSetsByValue = new();
    private readonly Dictionary<ContextualPeerSetKey, FloatRange?> _detachedContextualRanges = new();
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

    public void Refresh(IReadOnlyList<InstanceId> changedInstanceIds)
    {
        ArgumentNullException.ThrowIfNull(changedInstanceIds);

        var affectedInstanceIds = changedInstanceIds.ToHashSet();

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
                peerSet.UpdateEffectiveRanges(value, false);
                continue;
            }

            foreach (var peer in peerSet.Values)
            {
                peer.SetPeers(peerSet);
                assigned.Add(peer);
            }
            peerSet.UpdateEffectiveRanges(value, true);
        }

        RefreshContextualPeerSets(affectedInstanceIds);
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

            _peerSetsByValue.TryAdd(value, new HashSet<PeerSet>());
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

            if (_peerSetsByValue.Remove(value, out var peerSets))
            {
                foreach (var peerSet in peerSets.ToArray())
                {
                    if (peerSet.Source is not null &&
                        !ReferenceEquals(peerSet.Source, value))
                    {
                        _detachedContextualRanges[peerSet.Key] =
                            peerSet.EffectiveRange;
                    }
                    RemoveContextualPeerSet(peerSet.Key);
                }
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

        IObservedValue[] changedSources;
        lock (_lock)
        {
            changedSources = _peerSetsByValue.TryGetValue(
                    changedValue,
                    out var peerSets)
                ? peerSets
                    .Where(peerSet =>
                        peerSet.UpdateEffectiveRanges(peerSet.Source!, false) &&
                        !ReferenceEquals(peerSet.Source, changedValue))
                    .Select(peerSet => peerSet.Source!)
                    .Distinct()
                    .ToArray()
                : Array.Empty<IObservedValue>();
        }
        foreach (var source in changedSources)
        {
            source.NotifyEffectiveRangeChanged();
        }
    }

    private IReadOnlyList<IObservedValue> ResolvePeers(IObservedValue value)
    {
        if (value.Scope is StateValueEditScope.Local || value.Bank is null)
        {
            return [value];
        }

        if (value.Scope is not StateValueEditScope.BankGroup ||
            value.Bank is not { } bank)
        {
            throw new InvalidOperationException(
                $"State path {value.Path} cannot use bank-group peer editing.");
        }

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

    private PeerSet ResolveMutationPeers(IObservedValue value)
    {
        if (value.Scope is not StateValueEditScope.BankGroup ||
            value.Bank is not null ||
            !_isEditing)
        {
            return value.Peers;
        }

        return GetContextualPeerSet(value, _editingBank);
    }

    private PeerSet ResolveContextualPeers(
        IObservedValue value,
        BankAddress? focusedBank)
    {
        if (value.Scope is not StateValueEditScope.BankGroup ||
            value.Bank is not null ||
            focusedBank is null)
        {
            return value.Peers;
        }

        return GetContextualPeerSet(value, focusedBank);
    }

    private PeerSet GetContextualPeerSet(
        IObservedValue value,
        BankAddress? focusedBank)
    {
        var key = new ContextualPeerSetKey(
            value.InstanceId,
            value.Path,
            value.ValueType,
            focusedBank);
        lock (_lock)
        {
            if (_contextualPeerSets.TryGetValue(key, out var existing))
            {
                return existing;
            }

            var peerSet = new PeerSet(
                key,
                value,
                ResolveFocusedPeers(value, focusedBank),
                false);
            _contextualPeerSets.Add(key, peerSet);
            foreach (var peer in peerSet.Values)
            {
                if (!_peerSetsByValue.TryGetValue(peer, out var peerSets))
                {
                    peerSets = new HashSet<PeerSet>();
                    _peerSetsByValue.Add(peer, peerSets);
                }
                peerSets.Add(peerSet);
            }
            peerSet.UpdateEffectiveRanges(value, false);
            return peerSet;
        }
    }

    private void RefreshContextualPeerSets(
        IEnumerable<InstanceId> affectedInstanceIds)
    {
        var affected = affectedInstanceIds.ToHashSet();
        var changedSources = new HashSet<IObservedValue>();
        lock (_lock)
        {
            var previousRanges = _contextualPeerSets
                .Where(entry => affected.Contains(entry.Key.InstanceId) ||
                    entry.Value.Values.Any(value => affected.Contains(value.InstanceId)))
                .ToDictionary(
                    entry => entry.Key,
                    entry => entry.Value.EffectiveRange);
            foreach (var entry in _detachedContextualRanges)
            {
                previousRanges.TryAdd(entry.Key, entry.Value);
            }
            _detachedContextualRanges.Clear();
            foreach (var key in previousRanges.Keys)
            {
                RemoveContextualPeerSet(key);
            }

            foreach (var entry in previousRanges)
            {
                if (!_valuesByAddress.TryGetValue(
                        new ObservedValueAddress(
                            entry.Key.InstanceId,
                            entry.Key.Path),
                        out var value))
                {
                    continue;
                }

                var peerSet = GetContextualPeerSet(
                    value,
                    entry.Key.FocusedBank);
                if (entry.Value != peerSet.EffectiveRange)
                {
                    changedSources.Add(value);
                }
            }
        }
        foreach (var source in changedSources)
        {
            source.NotifyEffectiveRangeChanged();
        }
    }

    private void RemoveContextualPeerSet(ContextualPeerSetKey key)
    {
        if (!_contextualPeerSets.Remove(key, out var peerSet))
        {
            return;
        }

        foreach (var value in peerSet.Values)
        {
            if (_peerSetsByValue.TryGetValue(value, out var peerSets))
            {
                peerSets.Remove(peerSet);
            }
        }
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

        FloatRange CalculateEffectiveDeltaRange(
            IReadOnlyList<IObservedValue> peers);

        FloatRange? CalculateEffectiveRange(FloatRange deltaRange);

        void SetEffectiveDeltaRange(FloatRange range, bool notify);

        void NotifyEffectiveRangeChanged();
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
        private FloatRange? _effectiveRange;

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

            return focusedBank is null
                ? _effectiveRange
                : _owner.ResolveContextualPeers(this, focusedBank).EffectiveRange;
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
            if (_value is null)
            {
                return;
            }

            _owner.Detach(this);
            _value = null;
            _peers = PeerSet.Empty;
        }

        public void SetPeers(PeerSet peers)
        {
            ArgumentNullException.ThrowIfNull(peers);
            _peers = peers;
        }

        public FloatRange CalculateEffectiveDeltaRange(
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

        public FloatRange? CalculateEffectiveRange(FloatRange deltaRange)
        {
            if (_editMode is not StateValueEditMode.ApplyDelta ||
                _value is null ||
                !deltaRange.IsValid)
            {
                return null;
            }

            var currentValue = (float)(object)_value.Value!;
            return new FloatRange(
                currentValue + deltaRange.Minimum,
                currentValue + deltaRange.Maximum);
        }

        public void SetEffectiveDeltaRange(FloatRange range, bool notify)
        {
            var effectiveRange = CalculateEffectiveRange(range);
            var changed = _effectiveRange != effectiveRange;
            _effectiveRange = effectiveRange;
            if (changed && notify && _value is not null)
            {
                _effectiveRangeChanged(_value.Value);
            }
        }

        public void NotifyEffectiveRangeChanged()
        {
            if (_value is not null)
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

    private readonly record struct ContextualPeerSetKey(
        InstanceId InstanceId,
        StatePath Path,
        Type ValueType,
        BankAddress? FocusedBank);

    private sealed class PeerSet
    {
        private object?[]? _effectiveRangeValues;
        private readonly bool _sharesEffectiveRange;
        private readonly bool _tracksEffectiveRange;
        private StateHistoryTransaction? _pendingEffectiveRangeRefresh;

        public PeerSet(
            IReadOnlyList<IObservedValue> values,
            bool sharesEffectiveRange)
            : this(default, null!, values, sharesEffectiveRange)
        {
        }

        public PeerSet(
            ContextualPeerSetKey key,
            IObservedValue source,
            IReadOnlyList<IObservedValue> values,
            bool sharesEffectiveRange)
        {
            ArgumentNullException.ThrowIfNull(values);
            Key = key;
            Source = source;
            Values = values;
            _sharesEffectiveRange = sharesEffectiveRange;
            _tracksEffectiveRange = values.Any(value =>
                value.TracksEffectiveRange);
        }

        public static PeerSet Empty { get; } =
            new(default, null!, Array.Empty<IObservedValue>(), true);

        public ContextualPeerSetKey Key { get; }

        public IObservedValue? Source { get; }

        public IReadOnlyList<IObservedValue> Values { get; }

        public FloatRange? EffectiveRange { get; private set; }

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

        public bool UpdateEffectiveRanges(
            IObservedValue source,
            bool notify)
        {
            if (!_tracksEffectiveRange)
            {
                return false;
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
                    return false;
                }
            }

            _effectiveRangeValues = Values
                .Select(value => value.GetCurrentValue())
                .ToArray();
            var effectiveDeltaRange = source.CalculateEffectiveDeltaRange(Values);
            var effectiveRange = source.CalculateEffectiveRange(effectiveDeltaRange);
            var changed = EffectiveRange != effectiveRange;
            EffectiveRange = effectiveRange;
            if (Source is not null)
            {
                return changed;
            }

            IReadOnlyList<IObservedValue> targets = _sharesEffectiveRange
                ? Values
                : [source];
            foreach (var value in targets)
            {
                value.SetEffectiveDeltaRange(effectiveDeltaRange, notify);
            }
            return changed;
        }
    }
}

internal interface IStatePeerValueObserver<TValue> : IStateValueObserver<TValue>
{
    FloatRange? GetEffectiveRange(BankAddress? focusedBank = null);
}

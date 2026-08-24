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
    private readonly List<IObservedValue> _values = new();

    public StatePeerObserver(
        StateHistory history,
        TopologyIndex topology)
    {
        _history = history;
        _topology = topology;
    }

    public IStateValueObserver<TValue> Create<TValue>(
        InstanceId instanceId,
        StatePath path,
        StateValueEditScope scope,
        StateValueEditMode editMode,
        FloatRange? physicalRange)
    {
        Validate<TValue>(editMode, physicalRange);
        return new ObservedValue<TValue>(
            this,
            instanceId,
            path,
            scope,
            editMode,
            physicalRange);
    }

    public void Refresh(InstanceId changedInstanceId)
    {
        Refresh([changedInstanceId]);
    }

    public void Refresh(IReadOnlyList<InstanceId> changedInstanceIds)
    {
        ArgumentNullException.ThrowIfNull(changedInstanceIds);

        IObservedValue[] values;
        var affectedInstanceIds = changedInstanceIds.ToHashSet();
        lock (_lock)
        {
            values = _values.ToArray();
        }

        foreach (var value in values)
        {
            if (affectedInstanceIds.Contains(value.InstanceId) ||
                changedInstanceIds.Any(value.ContainsPeer))
            {
                affectedInstanceIds.Add(value.InstanceId);
            }
        }

        foreach (var changedInstanceId in changedInstanceIds)
        {
            affectedInstanceIds.UnionWith(
                _topology.ResolveStatePeerInstanceIds(changedInstanceId));
        }

        foreach (var value in values)
        {
            if (affectedInstanceIds.Contains(value.InstanceId))
            {
                value.SetPeers(ResolvePeers(value, values));
            }
        }
    }

    private void Attach(IObservedValue value)
    {
        lock (_lock)
        {
            _values.Add(value);
        }

        Refresh(value.InstanceId);
    }

    private void Detach(IObservedValue value)
    {
        lock (_lock)
        {
            _values.Remove(value);
        }
    }

    private void ValueChanged(IObservedValue changedValue)
    {
        IObservedValue[] component;
        lock (_lock)
        {
            component = _values
                .Where(value =>
                    value.ValueType == changedValue.ValueType &&
                    value.PeerPath.Equals(changedValue.PeerPath) &&
                    (ReferenceEquals(value, changedValue) ||
                        value.ContainsPeer(changedValue.InstanceId)))
                .ToArray();
        }

        foreach (var value in component)
        {
            value.UpdateEffectiveRange();
        }
    }

    private IReadOnlyList<IObservedValue> ResolvePeers(
        IObservedValue value,
        IReadOnlyList<IObservedValue> values)
    {
        if (value.Scope is StateValueEditScope.Local)
        {
            return [value];
        }

        if (value.Bank is { } bank)
        {
            var connectedBanks = _topology
                .GetConnectedGroupBanks([bank])
                .ToHashSet();
            return values
                .Where(candidate =>
                    candidate.ValueType == value.ValueType &&
                    candidate.PeerPath.Equals(value.PeerPath) &&
                    candidate.Bank is { } candidateBank &&
                    connectedBanks.Contains(candidateBank))
                .ToArray();
        }

        var targetIds = _topology.ResolveStatePeerInstanceIds(value.InstanceId);
        return values
            .Where(candidate =>
                candidate.ValueType == value.ValueType &&
                candidate.PeerPath.Equals(value.PeerPath) &&
                targetIds.Contains(candidate.InstanceId))
            .ToArray();
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

        bool ContainsPeer(InstanceId instanceId);

        void SetPeers(IReadOnlyList<IObservedValue> peers);

        void UpdateEffectiveRange();
    }

    private sealed class ObservedValue<TValue> :
        IObservedValue,
        IStateValueObserver<TValue>
    {
        private readonly StatePeerObserver _owner;
        private readonly StateValueEditMode _editMode;
        private readonly FloatRange? _physicalRange;
        private StateValue<TValue>? _value;
        private IObservedValue[] _peers = Array.Empty<IObservedValue>();
        private FloatRange _effectiveDeltaRange = new(1.0F, 0.0F);

        public ObservedValue(
            StatePeerObserver owner,
            InstanceId instanceId,
            StatePath path,
            StateValueEditScope scope,
            StateValueEditMode editMode,
            FloatRange? physicalRange)
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
        }

        public InstanceId InstanceId { get; }

        public StatePath Path { get; }

        public StatePath PeerPath { get; }

        public BankAddress? Bank { get; }

        public Type ValueType => typeof(TValue);

        public StateValueEditScope Scope { get; }

        public bool ContainsPeer(InstanceId instanceId) =>
            _peers.Any(peer => peer.InstanceId == instanceId);

        public void Attach(StateValue<TValue> value)
        {
            _value = value;
            value.SetMutationHandler(Set);
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
            _peers = Array.Empty<IObservedValue>();
        }

        public void SetPeers(IReadOnlyList<IObservedValue> peers)
        {
            _peers = peers.ToArray();
            UpdateEffectiveRange();
        }

        public void UpdateEffectiveRange()
        {
            if (_editMode is not StateValueEditMode.ApplyDelta)
            {
                return;
            }

            var minimum = float.NegativeInfinity;
            var maximum = float.PositiveInfinity;
            foreach (var peer in _peers.Cast<ObservedValue<TValue>>())
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

            _effectiveDeltaRange = _peers.Length == 0
                ? new FloatRange(1.0F, 0.0F)
                : new FloatRange(minimum, maximum);
        }

        private void Set(TValue value)
        {
            if (_peers.Length == 0)
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

            var delta = _editMode is StateValueEditMode.ApplyDelta
                ? Subtract(value, _value.Value)
                : default;
            if (_editMode is StateValueEditMode.ApplyDelta &&
                !_effectiveDeltaRange.Contains((float)(object)delta!))
            {
                throw new InvalidOperationException(
                    $"The requested delta is outside the effective range for path {Path}.");
            }

            foreach (var peer in _peers.Cast<ObservedValue<TValue>>())
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
}

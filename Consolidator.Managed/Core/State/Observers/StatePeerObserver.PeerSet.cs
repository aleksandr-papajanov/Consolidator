using Consolidator.Managed.State.History;


namespace Consolidator.Managed.Core.State.Observers;

internal sealed partial class StatePeerObserver
{
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
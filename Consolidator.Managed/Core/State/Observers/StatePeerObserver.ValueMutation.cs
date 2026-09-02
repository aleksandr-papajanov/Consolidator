using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed partial class StatePeerObserver
{
    private sealed partial class StatePeerValueObserver<TValue>
    {
        private void Set(TValue value)
        {
            if (_peers.Values.Count == 0)
            {
                throw new InvalidOperationException(
                    $"No state peers are available for path {Path}.");
            }

            using var transaction = _owner._history.BeginTransaction();
            Prepare(value, StateValueEditMode.CopyValue, transaction);
            transaction.Commit();
        }

        private void Prepare(
            TValue value,
            StateValueEditMode editMode,
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

            Validate<TValue>(editMode, _physicalRange);
            var delta = editMode is StateValueEditMode.ApplyDelta
                ? Subtract(value, _value.Value)
                : default;
            if (editMode is StateValueEditMode.ApplyDelta &&
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
                    : editMode is StateValueEditMode.ApplyDelta
                        ? Add(peer._value.Value, delta!)
                        : value;
                peer._value.Prepare(peerValue, transaction);
            }
        }

        private int PrepareReset(StateHistoryTransaction transaction)
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

            peers.ScheduleEffectiveRangeRefresh(transaction, this);
            var resetCount = 0;
            foreach (var peer in peers.Values.Cast<StatePeerValueObserver<TValue>>())
            {
                if (peer._value is null)
                {
                    throw new InvalidOperationException(
                        $"A state peer was removed for path {Path}.");
                }

                if (peer._value.PrepareResetDirect(transaction))
                {
                    resetCount++;
                }
            }

            return resetCount;
        }
    }
}
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed partial class StatePeerObserver
{
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
}
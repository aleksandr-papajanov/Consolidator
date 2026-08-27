using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed class DspStateObserver<TValue> : IStateValueObserver<TValue>
{
    private readonly DspStateChangeTracker _changes;
    private readonly InstanceId _instanceId;

    public DspStateObserver(
        DspStateChangeTracker changes,
        InstanceId instanceId)
    {
        ArgumentNullException.ThrowIfNull(changes);

        _changes = changes;
        _instanceId = instanceId;
    }

    public void Attach(StateValue<TValue> value)
    {
    }

    public void ValueChanged(
        StateValue<TValue> value,
        TValue previousValue,
        TValue currentValue)
    {
        _changes.MarkChanged(_instanceId);
    }

    public void Detach(StateValue<TValue> value)
    {
    }
}

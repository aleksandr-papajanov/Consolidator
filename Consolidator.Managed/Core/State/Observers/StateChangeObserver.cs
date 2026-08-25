using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed class StateChangeObserver<TValue> : IStateValueObserver<TValue>
{
    private readonly InstanceId _instanceId;
    private readonly StatePath _path;
    private readonly StateValueOwnership _ownership;
    private readonly IStateChangeSink _sink;

    public StateChangeObserver(
        InstanceId instanceId,
        StatePath path,
        StateValueOwnership ownership,
        IStateChangeSink sink)
    {
        _instanceId = instanceId;
        _path = path;
        _ownership = ownership;
        _sink = sink;
    }

    public void Attach(StateValue<TValue> value)
    {
    }

    public void ValueChanged(
        StateValue<TValue> value,
        TValue previousValue,
        TValue currentValue)
    {
        _sink.Publish(new StateValueChanged(
            _instanceId,
            _path,
            _ownership,
            previousValue,
            currentValue));
    }

    public void EffectiveRangeChanged(TValue currentValue)
    {
        _sink.Publish(new StateValueChanged(
            _instanceId,
            _path,
            _ownership,
            currentValue,
            currentValue,
            false));
    }

    public void Detach(StateValue<TValue> value)
    {
    }
}

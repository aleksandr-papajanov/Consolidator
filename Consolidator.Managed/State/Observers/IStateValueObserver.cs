namespace Consolidator.Managed.State.Observers;

public interface IStateValueObserver<TValue>
{
    void Attach(StateValue<TValue> value);

    void ValueChanged(
        StateValue<TValue> value,
        TValue previousValue,
        TValue currentValue);

    void Detach(StateValue<TValue> value);
}

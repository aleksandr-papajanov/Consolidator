using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Observers;

public sealed class StateProjectionObserver<TValue> : IStateValueObserver<TValue>
{
    private readonly Action<TValue> _project;

    public StateProjectionObserver(Action<TValue> project)
    {
        ArgumentNullException.ThrowIfNull(project);
        _project = project;
    }

    public void Attach(StateValue<TValue> value)
    {
        ArgumentNullException.ThrowIfNull(value);
        _project(value.Value);
    }

    public void ValueChanged(
        StateValue<TValue> value,
        TValue previousValue,
        TValue currentValue)
    {
        _project(currentValue);
    }

    public void Detach(StateValue<TValue> value)
    {
    }
}

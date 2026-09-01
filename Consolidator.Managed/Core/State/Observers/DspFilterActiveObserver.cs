using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed class DspFilterActiveObserver : IStateValueObserver<bool>
{
    private readonly Action<bool> _setActive;

    public DspFilterActiveObserver(Action<bool> setActive)
    {
        ArgumentNullException.ThrowIfNull(setActive);
        _setActive = setActive;
    }

    public void Attach(StateValue<bool> value)
    {
        _setActive(!value.Value);
    }

    public void ValueChanged(
        StateValue<bool> value,
        bool previousValue,
        bool currentValue)
    {
        _setActive(!currentValue);
    }

    public void Detach(StateValue<bool> value)
    {
    }
}
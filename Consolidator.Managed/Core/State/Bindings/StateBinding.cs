namespace Consolidator.Managed.Core.State.Bindings;

public sealed class StateBinding<TValue> : IStateBinding<TValue>
{
    private readonly Action<TValue> _apply;

    public StateBinding(Action<TValue> apply)
    {
        ArgumentNullException.ThrowIfNull(apply);
        _apply = apply;
    }

    public void Apply(TValue value)
    {
        _apply(value);
    }
}

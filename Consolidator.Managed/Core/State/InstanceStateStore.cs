namespace Consolidator.Managed.Core.State;

public sealed class InstanceStateStore : IDisposable
{
    private readonly Dictionary<StateId, IStateValue> _values = new();

    public StateValue<TValue> Add<TValue>(StateValue<TValue> value)
    {
        ArgumentNullException.ThrowIfNull(value);
        _values.Add(value.Id, value);
        return value;
    }

    public StateValue<TValue>? Find<TValue>(StateId id)
    {
        if (!_values.TryGetValue(id, out var value))
        {
            return null;
        }

        return value as StateValue<TValue>
            ?? throw new InvalidOperationException(
                $"State value {id.Value} has a different type.");
    }

    public void Dispose()
    {
        foreach (var value in _values.Values)
        {
            value.Dispose();
        }

        _values.Clear();
    }
}

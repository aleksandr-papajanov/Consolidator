namespace Consolidator.Managed.Core.Instances;

public sealed class InstanceRegistry
{
    private readonly Dictionary<ulong, ConsolidatorInstance> _instances = new();
    private readonly object _lock = new();

    public void Add(ConsolidatorInstance instance)
    {
        ArgumentNullException.ThrowIfNull(instance);

        lock (_lock)
        {
            _instances.Add(instance.Id, instance);
        }
    }

    public ConsolidatorInstance? Find(ulong instanceId)
    {
        lock (_lock)
        {
            return _instances.TryGetValue(instanceId, out var instance)
                ? instance
                : null;
        }
    }

    public ConsolidatorInstance? Remove(ulong instanceId)
    {
        lock (_lock)
        {
            return _instances.Remove(instanceId, out var instance)
                ? instance
                : null;
        }
    }

    public ConsolidatorInstance[] GetAll()
    {
        lock (_lock)
        {
            return _instances.Values.ToArray();
        }
    }
}

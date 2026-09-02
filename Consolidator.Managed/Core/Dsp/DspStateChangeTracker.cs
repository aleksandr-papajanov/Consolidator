namespace Consolidator.Managed.Core.Dsp;

internal sealed class DspStateChangeTracker
{
    private readonly HashSet<InstanceId> _instances = [];
    private readonly object _lock = new();

    public void MarkChanged(InstanceId instanceId)
    {
        lock (_lock)
        {
            _instances.Add(instanceId);
        }
    }

    public IReadOnlyList<InstanceId> Drain()
    {
        lock (_lock)
        {
            var instances = _instances
                .OrderBy(instanceId => instanceId.Value)
                .ToArray();
            _instances.Clear();
            return instances;
        }
    }
}

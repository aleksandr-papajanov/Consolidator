using Consolidator.Managed.Core.Abstractions;
using Consolidator.Managed.Core.Instances;
using Consolidator.Managed.Protocol;

namespace Consolidator.Managed.Core;

public sealed class Coordinator
{
    private readonly IConsolidatorLogger _logger;
    private readonly Dictionary<ulong, ConsolidatorInstance> _instances = new();
    private readonly object _instanceLock = new();
    private ulong _nextInstanceId;

    public Coordinator(IConsolidatorLogger logger)
    {
        _logger = logger;
    }

    public ConsolidatorInstance RegisterInstance(
        IInstanceOutput output,
        IDspStatePublisher dspPublisher)
    {
        ArgumentNullException.ThrowIfNull(output);
        ArgumentNullException.ThrowIfNull(dspPublisher);

        ConsolidatorInstance instance;

        lock (_instanceLock)
        {
            var id = ++_nextInstanceId;
            instance = new ConsolidatorInstance(
                id,
                output,
                dspPublisher);
            _instances.Add(id, instance);
        }

        _logger.Info($"Registered instance {instance.Id}");

        return instance;
    }

    public void UnregisterInstance(
        ulong instanceId)
    {
        ConsolidatorInstance? instance;

        lock (_instanceLock)
        {
            if (!_instances.Remove(instanceId, out instance))
            {
                return;
            }
        }

        instance.Stop();
        _logger.Info($"Unregistered instance {instanceId}");
    }

    public void ReceiveMessage(
        ulong instanceId,
        string selector,
        ReadOnlySpan<Atom> atoms)
    {
    }

    public void Prepare(
        ulong instanceId,
        double sampleRate,
        nuint maximumFrameCount)
    {
        var instance = FindInstance(instanceId);

        if (instance is null)
        {
            return;
        }

        instance.Prepare(
            sampleRate,
            maximumFrameCount);
    }

    private ConsolidatorInstance? FindInstance(ulong instanceId)
    {
        lock (_instanceLock)
        {
            _instances.TryGetValue(instanceId, out var instance);
            return instance;
        }
    }
}
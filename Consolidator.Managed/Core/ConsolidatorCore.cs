using Consolidator.Managed.Core.Abstractions;
using Consolidator.Managed.Core.Instances;
using Consolidator.Managed.Protocol;

namespace Consolidator.Managed.Core;

public sealed class ConsolidatorCore
{
    private readonly Coordinator _coordinator;

    public ConsolidatorCore(Coordinator coordinator)
    {
        _coordinator = coordinator;
    }

    public ConsolidatorInstance RegisterInstance(
        IInstanceOutput output,
        IDspStatePublisher dspPublisher)
    {
        return _coordinator.RegisterInstance(output, dspPublisher);
    }

    public void UnregisterInstance(
        ulong instanceId)
    {
        _coordinator.UnregisterInstance(instanceId);
    }

    public void ReceiveMessage(
        ulong instanceId,
        string selector,
        ReadOnlySpan<Atom> atoms)
    {
        _coordinator.ReceiveMessage(instanceId, selector, atoms);
    }

    public void Prepare(
        ulong instanceId,
        double sampleRate,
        nuint maximumFrameCount)
    {
        _coordinator.Prepare(
            instanceId,
            sampleRate,
            maximumFrameCount);
    }

}
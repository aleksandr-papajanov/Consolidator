using Consolidator.Managed.Core.Abstractions;
using Consolidator.Managed.Protocol;

namespace Consolidator.Managed.Core.Instances;

public sealed class ConsolidatorInstance
{
    private readonly object _lifecycleLock = new();
    private readonly IInstanceOutput _output;
    private bool _active = true;

    public ConsolidatorInstance(
        ulong id,
        IInstanceOutput output)
    {
        Id = id;
        _output = output;
    }

    public ulong Id { get; }

    public bool TrySend(
        string selector,
        ReadOnlySpan<Atom> atoms)
    {
        lock (_lifecycleLock)
        {
            if (!_active)
            {
                return false;
            }

            _output.Send(selector, atoms);
            return true;
        }
    }

    public void Prepare(
        double sampleRate,
        nuint maximumFrameCount)
    {
    }

    public void Stop()
    {
        lock (_lifecycleLock)
        {
            _active = false;
        }
    }
}
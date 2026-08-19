using Consolidator.Managed.Native;
using Consolidator.Managed.Protocol;

namespace Consolidator.Managed.Core;

public sealed class ConsolidatorInstance
{
    private readonly object _lifecycleLock = new();
    private readonly NativeOutput _output;
    private bool _active = true;

    public ConsolidatorInstance(
        ulong id,
        NativeOutput output)
    {
        Id = id;
        _output = output;
    }

    public ulong Id { get; }

    public bool TrySend(
        string selector,
        ReadOnlySpan<Consolidator.Managed.Protocol.Atom> atoms)
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

    public void Stop()
    {
        lock (_lifecycleLock)
        {
            _active = false;
        }
    }
}
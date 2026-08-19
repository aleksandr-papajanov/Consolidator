using Consolidator.Managed.Native;
using Consolidator.Managed.Protocol;

namespace Consolidator.Managed.Core;

public sealed class ConsolidatorCore
{
    private readonly IConsolidatorLogger _logger;
    private readonly Dictionary<ulong, ConsolidatorInstance> _instances = new();
    private readonly object _instanceLock = new();
    private ulong _nextInstanceId;

    public ConsolidatorCore(IConsolidatorLogger logger)
    {
        _logger = logger;
    }

    public unsafe ulong RegisterInstance(
        void* context,
        delegate* unmanaged[Cdecl]<
            void*,
            byte*,
            NativeAtom*,
            nuint,
            void> outputCallback)
    {
        if (outputCallback == null)
        {
            return 0;
        }

        ulong id;

        lock (_instanceLock)
        {
            id = ++_nextInstanceId;
            var output = new NativeOutput(context, outputCallback);
            _instances.Add(id, new ConsolidatorInstance(id, output));
        }

        _logger.Info($"Registered instance {id}");

        return id;
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
        Atom[] atoms)
    {
        _logger.Info($"{instanceId}: {selector} ({atoms.Length} atoms)");

        ConsolidatorInstance? instance;
        lock (_instanceLock)
        {
            _instances.TryGetValue(instanceId, out instance);
        }

        if (instance is null)
        {
            return;
        }

        var response = new Atom[atoms.Length + 1];
        response[0] = new Atom(AtomType.Symbol, 0, 0, selector);
        Array.Copy(atoms, 0, response, 1, atoms.Length);
        instance.TrySend(
            "message_received",
            new ReadOnlySpan<Consolidator.Managed.Protocol.Atom>(response));
    }

    public void Prepare(
        ulong instanceId,
        double sampleRate,
        nuint maximumFrameCount)
    {
    }

    public unsafe void ReceiveAudio(
        ulong instanceId,
        double* mainLeft,
        double* mainRight,
        double* referenceLeft,
        double* referenceRight,
        nuint frameCount)
    {
        //
        // НИЧЕГО тяжёлого пока.
        //
        // Позже:
        // analyzerInput.Write(...)
        //
    }
}
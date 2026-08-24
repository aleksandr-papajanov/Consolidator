using System.Collections.Concurrent;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Native;

internal sealed class NativeOutputService : IProtocolTransport, IProtocolOutputRegistry
{
    private readonly ConcurrentDictionary<ulong, IProtocolOutputCallback> _callbacks = new();

    public void Register(
        ulong instanceId,
        IProtocolOutputCallback callback)
    {
        ArgumentNullException.ThrowIfNull(callback);

        if (!_callbacks.TryAdd(instanceId, callback))
        {
            throw new InvalidOperationException(
                $"Output is already registered for instance {instanceId}.");
        }
    }

    public void Unregister(ulong instanceId)
    {
        _callbacks.TryRemove(instanceId, out _);
    }

    public void Send(
        ProtocolOutput message)
    {
        ArgumentNullException.ThrowIfNull(message);

        foreach (var instanceId in message.TargetInstanceIds.Distinct())
        {
            if (_callbacks.TryGetValue(instanceId, out var callback))
            {
                callback.Send(message);
            }
        }
    }
}





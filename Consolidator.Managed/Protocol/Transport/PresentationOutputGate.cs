using System.Collections.Concurrent;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Transport;

internal sealed class PresentationOutputGate : IPresentationTransport
{
    private readonly IProtocolTransport _output;
    private readonly ConcurrentDictionary<ulong, byte> _activeInstances = new();

    public PresentationOutputGate(IProtocolTransport output)
    {
        ArgumentNullException.ThrowIfNull(output);
        _output = output;
    }

    public void Send(ProtocolOutput message)
    {
        ArgumentNullException.ThrowIfNull(message);

        foreach (var instanceId in message.TargetInstanceIds.Distinct())
        {
            if (message.DeliverySemantics != DeliverySemantics.ActivePresentation)
            {
                SendToInstance(instanceId, message);
                continue;
            }

            if (_activeInstances.ContainsKey(instanceId))
            {
                RuntimeMetrics.Shared.RecordPresentationActiveDelivery();
                SendToInstance(instanceId, message);
            }
            else
            {
                RuntimeMetrics.Shared.RecordPresentationDiscarded();
            }
        }
    }

    public void SetActive(ulong instanceId, bool active)
    {
        if (active)
        {
            _activeInstances[instanceId] = 1;
        }
        else
        {
            _activeInstances.TryRemove(instanceId, out _);
        }
    }

    public void Unregister(ulong instanceId)
    {
        _activeInstances.TryRemove(instanceId, out _);
    }

    private void SendToInstance(ulong instanceId, ProtocolOutput message)
    {
        _output.Send(message with
        {
            TargetInstanceIds = [instanceId]
        });
    }
}

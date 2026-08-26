using System.Collections.Concurrent;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Transport;

internal sealed class PresentationOutputGate : IPresentationTransport
{
    private readonly IProtocolTransport _output;
    private readonly ConcurrentDictionary<ulong, PresentationState> _states = new();
    private long _batchRevision;

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
            if (message.DeliverySemantics != DeliverySemantics.CoalescedPresentation)
            {
                SendToInstance(instanceId, message);
                continue;
            }

            if (string.IsNullOrEmpty(message.DeliveryKey))
            {
                throw new InvalidOperationException(
                    "Coalesced presentation output requires a delivery key.");
            }

            var state = _states.GetOrAdd(instanceId, static _ => new PresentationState());
            var sendImmediately = false;
            lock (state)
            {
                if (state.Active)
                {
                    sendImmediately = true;
                }
                else
                {
                    var wasPending = state.Pending.ContainsKey(CreateKey(instanceId, message));
                    state.Pending[CreateKey(instanceId, message)] = message with
                    {
                        TargetInstanceIds = [instanceId]
                    };
                    RuntimeMetrics.Shared.RecordPresentationCoalesced();
                    RuntimeMetrics.Shared.AddPresentationPendingPaths(
                        wasPending ? 0 : 1,
                        state.Pending.Count);
                }
            }

            if (sendImmediately)
            {
                RuntimeMetrics.Shared.RecordPresentationActiveDelivery();
                SendToInstance(instanceId, message);
            }
        }
    }

    public void SetActive(ulong instanceId, bool active)
    {
        var state = _states.GetOrAdd(instanceId, static _ => new PresentationState());
        ProtocolOutput[] pending;
        lock (state)
        {
            state.Active = active;
            if (!active)
            {
                return;
            }

            pending = state.Pending.Values.ToArray();
            state.Pending.Clear();
            RuntimeMetrics.Shared.AddPresentationPendingPaths(-pending.Length, 0);
        }

        if (pending.Length == 0)
        {
            return;
        }

        RuntimeMetrics.Shared.RecordPresentationFlush(pending.Length);

        var revision = (ulong)Interlocked.Increment(ref _batchRevision);
        _output.Send(new ProtocolOutput(
            [instanceId],
            "state_batch_begin",
            [
                new Atom(AtomType.Integer, 1, 0, null),
                new Atom(AtomType.Integer, (long)revision, 0, null),
                new Atom(AtomType.Integer, pending.Length, 0, null)
            ],
            DeliverySemantics.Lossless));

        foreach (var message in pending.OrderBy(message => message.DeliveryKey, StringComparer.Ordinal))
        {
            _output.Send(new ProtocolOutput(
                new[] { instanceId },
                "state_batch_entry",
                new[]
                {
                    new Atom(AtomType.Integer, 1, 0, null),
                    new Atom(AtomType.Integer, (long)revision, 0, null)
                }
                .Concat(message.Atoms.Skip(1))
                .ToArray(),
                DeliverySemantics.Lossless));
        }

        _output.Send(new ProtocolOutput(
            [instanceId],
            "state_batch_done",
            [
                new Atom(AtomType.Integer, 1, 0, null),
                new Atom(AtomType.Integer, (long)revision, 0, null)
            ],
            DeliverySemantics.Lossless));
    }

    public void SetObservedTarget(ulong instanceId, ulong targetInstanceId, int bankId)
    {
        var state = _states.GetOrAdd(instanceId, static _ => new PresentationState());
        lock (state)
        {
            state.ObservedTarget = new ObservedTarget(targetInstanceId, bankId);
            var removedCount = state.Pending.Count;
            state.Pending.Clear();
            RuntimeMetrics.Shared.AddPresentationPendingPaths(-removedCount, 0);
        }
    }

    public void Unregister(ulong instanceId)
    {
        if (_states.TryRemove(instanceId, out var state))
        {
            lock (state)
            {
                RuntimeMetrics.Shared.AddPresentationPendingPaths(-state.Pending.Count, 0);
                state.Pending.Clear();
            }
        }
    }

    private static string CreateKey(ulong instanceId, ProtocolOutput message)
    {
        return string.Concat(instanceId, "\u001f", message.DeliveryKey);
    }

    private void SendToInstance(ulong instanceId, ProtocolOutput message)
    {
        _output.Send(message with
        {
            TargetInstanceIds = [instanceId]
        });
    }

    private sealed class PresentationState
    {
        public bool Active;

        public ObservedTarget? ObservedTarget;

        public Dictionary<string, ProtocolOutput> Pending { get; } = new(StringComparer.Ordinal);
    }

    private sealed record ObservedTarget(ulong InstanceId, int BankId);
}
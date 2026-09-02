using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class PersistenceChangePublisher : IPersistenceChangeSink
{
    private readonly HashSet<InstanceId> _suppressedInstances = new();
    private readonly object _lock = new();
    private readonly IProtocolTransport _transport;
    private readonly IManagedLogger _logger;

    public PersistenceChangePublisher(
        IProtocolTransport transport,
        IManagedLogger logger)
    {
        ArgumentNullException.ThrowIfNull(transport);
        ArgumentNullException.ThrowIfNull(logger);

        _transport = transport;
        _logger = logger;
    }

    public void Publish(StateValueChanged change)
    {
        if (!change.IsValueChange ||
            change.Path.Nodes.Contains(StateNodeIds.Label))
        {
            return;
        }

        lock (_lock)
        {
            if (_suppressedInstances.Contains(change.InstanceId))
            {
                return;
            }
        }

        try
        {
            _transport.Send(new ProtocolOutput(
                [change.InstanceId.Value],
                "persistence_dirty",
                Array.Empty<Atom>()));
        }
        catch (Exception exception)
        {
            _logger.Warning(
                $"Failed to publish persistence change for {change.Path}: " +
                exception.Message);
        }
    }

    public IDisposable Suppress(InstanceId instanceId)
    {
        lock (_lock)
        {
            if (!_suppressedInstances.Add(instanceId))
            {
                throw new InvalidOperationException(
                    "Persistence notifications are already suppressed for the instance.");
            }
        }

        return new Suppression(this, instanceId);
    }

    private void Resume(InstanceId instanceId)
    {
        lock (_lock)
        {
            _suppressedInstances.Remove(instanceId);
        }
    }

    private sealed class Suppression : IDisposable
    {
        private PersistenceChangePublisher? _owner;
        private readonly InstanceId _instanceId;

        public Suppression(
            PersistenceChangePublisher owner,
            InstanceId instanceId)
        {
            _owner = owner;
            _instanceId = instanceId;
        }

        public void Dispose()
        {
            Interlocked.Exchange(ref _owner, null)?.Resume(_instanceId);
        }
    }
}
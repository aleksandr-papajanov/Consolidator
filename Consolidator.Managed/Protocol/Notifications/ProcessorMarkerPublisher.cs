using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Protocol.Notifications;

internal sealed class ProcessorMarkerPublisher : IDisposable, IProcessorMarkerSink
{
    private readonly InstanceRegistry _registry;
    private readonly ProcessorMarkerProjection _projection;
    private readonly IRegistryChangeSink _registryChanges;
    private readonly StateTopologyObserver _topologyObserver;
    private readonly IPresentationTransport _transport;
    private readonly Dictionary<ulong, IReadOnlyDictionary<MarkerKey, bool>> _markers = new();
    private readonly object _lock = new();

    public ProcessorMarkerPublisher(
        InstanceRegistry registry,
        ProcessorMarkerProjection projection,
        IRegistryChangeSink registryChanges,
        StateTopologyObserver topologyObserver,
        IPresentationTransport transport)
    {
        _registry = registry;
        _projection = projection;
        _registryChanges = registryChanges;
        _topologyObserver = topologyObserver;
        _transport = transport;

        _registryChanges.RegistryChangedEvent += RegistryChanged;
        _registryChanges.ObserverUnregisteredEvent += ObserverUnregistered;
        _topologyObserver.FocusedBankChangedEvent += FocusedBankChanged;
        _topologyObserver.TopologyChangedEvent += TopologyChanged;
    }

    public RegistrySnapshotResult Initialize(
        InstanceId viewerInstanceId,
        RegistrySnapshotResult snapshot)
    {
        var projected = _projection.Project(viewerInstanceId, snapshot);
        lock (_lock)
        {
            _markers[viewerInstanceId.Value] = CreateIndex(projected.ProcessorMarkers);
        }
        return projected;
    }

    public void Dispose()
    {
        _registryChanges.RegistryChangedEvent -= RegistryChanged;
        _registryChanges.ObserverUnregisteredEvent -= ObserverUnregistered;
        _topologyObserver.FocusedBankChangedEvent -= FocusedBankChanged;
        _topologyObserver.TopologyChangedEvent -= TopologyChanged;
    }

    private void RegistryChanged(string selector)
    {
        if (selector is not (
            "registry_instance_added" or
            "registry_bank_effect_changed" or
            "registry_processor_changed"))
        {
            return;
        }

        foreach (var observerId in _registryChanges.GetObserverIds())
        {
            Refresh(new InstanceId(observerId));
        }
    }

    private void TopologyChanged()
    {
        foreach (var observerId in _registryChanges.GetObserverIds())
        {
            Refresh(new InstanceId(observerId));
        }
    }

    private void FocusedBankChanged(InstanceId viewerInstanceId, BankAddress? focusedBank)
    {
        if (_registryChanges.GetObserverIds().Contains(viewerInstanceId.Value))
        {
            Refresh(viewerInstanceId);
        }
    }

    private void ObserverUnregistered(ulong viewerInstanceId)
    {
        lock (_lock)
        {
            _markers.Remove(viewerInstanceId);
        }
    }

    private void Refresh(InstanceId viewerInstanceId)
    {
        var projected = _projection.Project(viewerInstanceId, _registry.CaptureSnapshot());
        var current = CreateIndex(projected.ProcessorMarkers);
        IReadOnlyDictionary<MarkerKey, bool> previous;
        lock (_lock)
        {
            previous = _markers[viewerInstanceId.Value];
            _markers[viewerInstanceId.Value] = current;
        }

        var changes = projected.ProcessorMarkers
            .Where(marker =>
            {
                var key = new MarkerKey(marker.InstanceId, marker.ProcessorId);
                return !previous.TryGetValue(key, out var active) || active != marker.Active;
            })
            .GroupBy(marker => marker.InstanceId)
            .ToArray();
        if (changes.Length == 0)
        {
            return;
        }

        var atoms = new List<Atom> { Integer(1), Integer(changes.Length) };
        foreach (var instance in changes)
        {
            var markers = instance.ToArray();
            atoms.Add(Symbol(instance.Key.ToString()));
            atoms.Add(Integer(markers.Length));
            foreach (var marker in markers)
            {
                atoms.Add(Symbol(ProcessorIds.Encode(marker.ProcessorId)));
                atoms.Add(Integer(marker.Active ? 1 : 0));
            }
        }

        _transport.Send(new ProtocolOutput(
            [viewerInstanceId.Value],
            "registry_processor_markers_changed",
            atoms,
            DeliverySemantics.Lossless));
    }

    private static IReadOnlyDictionary<MarkerKey, bool> CreateIndex(
        IReadOnlyList<RegistryProcessorMarkerSnapshot> markers) =>
        markers.ToDictionary(
            marker => new MarkerKey(marker.InstanceId, marker.ProcessorId),
            marker => marker.Active);

    private static Atom Integer(long value) => new(AtomType.Integer, value, 0, null);
    private static Atom Symbol(string value) => new(AtomType.Symbol, 0, 0, value);

    private readonly record struct MarkerKey(ulong InstanceId, ProcessorId ProcessorId);
}

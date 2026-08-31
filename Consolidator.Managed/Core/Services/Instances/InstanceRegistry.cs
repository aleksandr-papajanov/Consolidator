using Consolidator.Managed.Analyzer;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.PerInstance;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.Protocol.Notifications;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Services.Instances;

public sealed class InstanceRegistry : IDisposable, IInstanceLifecycleService
{
    private readonly Dictionary<InstanceId, ManagedInstance> _instances = new();
    private readonly object _lock = new();
    private readonly StateRegistry<InstanceId> _stateRegistry;
    private readonly StateValueFactory _stateValueFactory;
    private readonly StateTopologyObserver _topologyObserver;
    private readonly AudibilityObserver _audibilityObserver;
    private readonly DspStateChangeTracker _dspChanges;
    private readonly IOperationGate _operationGate;
    private ulong _nextInstanceId;
    private readonly RegistryChangePublisher _registryChanges;
    private readonly FftAnalyzer _fftAnalyzer;
    private readonly IActivityStatusSink _activityStatusSink;

    internal InstanceRegistry(
        StateRegistry<InstanceId> stateRegistry,
        StateValueFactory stateValueFactory,
        StateTopologyObserver topologyObserver,
        AudibilityObserver audibilityObserver,
        DspStateChangeTracker dspChanges,
        IOperationGate operationGate,
        RegistryChangePublisher registryChanges,
        FftAnalyzer fftAnalyzer,
        IActivityStatusSink activityStatusSink)
    {
        _stateRegistry = stateRegistry;
        _stateValueFactory = stateValueFactory;
        _topologyObserver = topologyObserver;
        _audibilityObserver = audibilityObserver;
        _dspChanges = dspChanges;
        _operationGate = operationGate;
        _registryChanges = registryChanges;
        _fftAnalyzer = fftAnalyzer;
        _activityStatusSink = activityStatusSink;
    }

    public InstanceId RegisterInstance(
        IDspStatePublisher dspPublisher)
    {
        ArgumentNullException.ThrowIfNull(dspPublisher);

        using (EnterOperation())
        {
            var instanceId = new InstanceId(++_nextInstanceId);
            var state = CreateState(instanceId);
            var instance = new ManagedInstance(
                instanceId,
                state,
                dspPublisher);

            lock (_lock)
            {
                _instances.Add(instanceId, instance);
            }

            instance.PublishDspState();

            PublishDspStates(_dspChanges.Drain()
                .Where(changedInstanceId => changedInstanceId != instanceId)
                .ToArray());

            _registryChanges.InstanceAdded(
                instanceId.Value,
                state.Instance.Label.Value,
                state.Instance.Mute.Value,
                state.Instance.Solo.Value,
                state.Activity.Snapshot(),
                state.Instance.Banks
                    .Select(bank => (
                        (int)bank.Id,
                        bank.Group.Value?.Value,
                        state.Dsp.EqualizerBanks[(int)bank.Id].EffectActive))
                    .ToArray());

            return instanceId;
        }
    }

    public void UnregisterInstance(InstanceId instanceId)
    {
        ManagedInstance instance;
        using (EnterOperation())
        {
            lock (_lock)
            {
                if (!_instances.Remove(instanceId, out instance!))
                {
                    return;
                }

                _stateRegistry.RemoveRoot(instanceId);
                _fftAnalyzer.RemoveInstance(instanceId);
            }

            _topologyObserver.RemoveState(instanceId);
            PublishDspStates(_dspChanges.Drain());
        }

        instance.Dispose();

        _registryChanges.UnregisterObserver(instanceId.Value);
        _registryChanges.InstanceRemoved(instanceId.Value);
    }

    internal ManagedInstance? FindInstance(InstanceId instanceId)
    {
        lock (_lock)
        {
            return _instances.TryGetValue(instanceId, out var instance)
                ? instance
                : null;
        }
    }

    internal bool Contains(InstanceId instanceId)
    {
        lock (_lock)
        {
            return _instances.ContainsKey(instanceId);
        }
    }

    internal IReadOnlyList<ulong> GetInstanceIds()
    {
        lock (_lock)
        {
            return _instances.Keys
                .Select(instanceId => instanceId.Value)
                .OrderBy(instanceId => instanceId)
                .ToArray();
        }
    }

    internal void PublishDspStates(IReadOnlyList<InstanceId> affectedInstanceIds)
    {
        lock (_lock)
        {
            foreach (var instanceId in affectedInstanceIds.Distinct())
            {
                if (_instances.TryGetValue(instanceId, out var instance))
                {
                    instance.State.Activity.Refresh();
                    instance.PublishDspState();
                }
            }
        }
    }

    internal void PublishAnalyzerState(
        InstanceId instanceId,
        ProcessorId snapshotContext)
    {
        lock (_lock)
        {
            if (_instances.TryGetValue(instanceId, out var instance))
            {
                _fftAnalyzer.PublishEqualizerState(
                    instance.State,
                    snapshotContext);
            }
        }
    }

    internal void PublishFilterCatalog(
        InstanceId instanceId,
        ProcessorId snapshotContext)
    {
        lock (_lock)
        {
            if (_instances.TryGetValue(instanceId, out var instance))
            {
                _fftAnalyzer.PublishFilterCatalog(
                    instance.State,
                    snapshotContext);
            }
        }
    }

    internal void PublishAnalyzerStates(IReadOnlyList<InstanceId> instanceIds)
    {
        lock (_lock)
        {
            foreach (var instanceId in instanceIds.Distinct())
            {
                if (_instances.TryGetValue(instanceId, out var instance))
                {
                    _fftAnalyzer.PublishEqualizerState(
                        instance.State,
                        instance.State.Transient.Selection.SelectedProcessor);
                }
            }
        }
    }

    internal RegistrySnapshotResult CreateSnapshot()
    {
        RuntimeMetrics.Shared.RecordRegistrySnapshot();
        return CaptureSnapshot();
    }

    internal RegistrySnapshotResult CaptureSnapshot()
    {
        ManagedInstance[] instances;
        lock (_lock)
        {
            instances = _instances.Values
                .OrderBy(instance => instance.InstanceId.Value)
                .ToArray();
        }

        var snapshots = instances.Select(instance => new RegistryInstanceSnapshot(
                instance.InstanceId.Value,
                instance.State.Instance.Label.Value,
                instance.State.Instance.Mute.Value,
                instance.State.Instance.Solo.Value,
                instance.State.Activity.Snapshot(),
                instance.State.Instance.Banks
                    .Select(bank => new RegistryBankSnapshot(
                        (int)bank.Id,
                        bank.Group.Value?.Value,
                        instance.State.Dsp.EqualizerBanks[(int)bank.Id].EffectActive))
                    .ToArray()))
            .ToArray();
        var groups = snapshots
            .SelectMany(instance => instance.Banks
                .Where(bank => bank.GroupId is not null)
                .Select(bank => new
                {
                    GroupId = bank.GroupId!.Value,
                    Member = new RegistryGroupMemberSnapshot(
                        instance.InstanceId,
                        bank.BankId)
                }))
            .GroupBy(entry => entry.GroupId)
            .OrderBy(group => group.Key)
            .Select(group => new RegistryGroupSnapshot(
                group.Key,
                group.Select(entry => entry.Member).ToArray()))
            .ToArray();
        return new RegistrySnapshotResult(
            _registryChanges.Revision,
            snapshots,
            groups,
            Array.Empty<RegistryProcessorMarkerSnapshot>());
    }

    public void Dispose()
    {
        ManagedInstance[] instances;

        using (EnterOperation())
        {
            lock (_lock)
            {
                instances = _instances.Values.ToArray();
                _instances.Clear();
            }

            foreach (var instance in instances)
            {
                _stateRegistry.RemoveRoot(instance.InstanceId);
                _fftAnalyzer.RemoveInstance(instance.InstanceId);
                _topologyObserver.RemoveState(instance.InstanceId);
            }
        }

        Exception? disposalError = null;
        foreach (var instance in instances)
        {
            try
            {
                instance.Dispose();
            }
            catch (Exception exception)
            {
                disposalError ??= exception;
            }

        }

        if (disposalError is not null)
        {
            throw new InvalidOperationException(
                "One or more instances failed to dispose.",
                disposalError);
        }
    }

    private ManagedState CreateState(InstanceId instanceId)
    {
        _stateRegistry.CreateRoot(instanceId);
        try
        {
            var runtime = DspDefaults.CreateRuntime();
            var transient = new InstanceTransientState(
                instanceId,
                _topologyObserver);
            var instance = new InstanceState(
                instanceId,
                _stateValueFactory,
                runtime,
                _audibilityObserver,
                _topologyObserver);
            var dsp = new DspState(
                instanceId,
                _stateValueFactory,
                runtime,
                _activityStatusSink);
            var root = _stateRegistry.GetRoot(instanceId);
            var state = new ManagedState(
                instance,
                transient,
                dsp,
                dsp.Activity,
                runtime,
                root);
            _topologyObserver.AddState(state.Instance, state.Transient);
            return state;
        }
        catch
        {
            _stateRegistry.RemoveRoot(instanceId);
            throw;
        }
    }

    private IDisposable EnterOperation()
    {
        return _operationGate.Enter();
    }
}




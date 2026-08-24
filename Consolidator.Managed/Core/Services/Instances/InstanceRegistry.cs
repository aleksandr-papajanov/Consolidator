using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.PerInstance;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.Services.Instances;

public sealed class InstanceRegistry : IDisposable, IInstanceLifecycleService
{
    private readonly Dictionary<InstanceId, ManagedInstance> _instances = new();
    private readonly object _lock = new();
    private readonly IManagedLogger _logger;
    private readonly StateRegistry<InstanceId> _stateRegistry;
    private readonly StateValueFactory _stateValueFactory;
    private readonly StateTopologyObserver _topologyObserver;
    private readonly AudibilityObserver _audibilityObserver;
    private readonly IOperationGate _operationGate;
    private ulong _nextInstanceId;

    internal InstanceRegistry(
        IManagedLogger logger,
        StateRegistry<InstanceId> stateRegistry,
        StateValueFactory stateValueFactory,
        StateTopologyObserver topologyObserver,
        AudibilityObserver audibilityObserver,
        IOperationGate operationGate)
    {
        _logger = logger;
        _stateRegistry = stateRegistry;
        _stateValueFactory = stateValueFactory;
        _topologyObserver = topologyObserver;
        _audibilityObserver = audibilityObserver;
        _operationGate = operationGate;
    }

    public InstanceId RegisterInstance(
        IDspStatePublisher dspPublisher)
    {
        ArgumentNullException.ThrowIfNull(dspPublisher);

        using (EnterOperation())
        {
            var instanceId = new InstanceId(++_nextInstanceId);
            var state = CreateState(instanceId);
            var instance = new ManagedInstance(instanceId, state, dspPublisher);
            lock (_lock)
            {
                _instances.Add(instanceId, instance);
            }

            instance.PublishDspState();
            _logger.Info($"Registered instance {instanceId}");
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
            }

            _topologyObserver.RemoveState(instanceId);
        }

        instance.Dispose();
        _logger.Info($"Unregistered instance {instanceId}");
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

    internal void PublishDspStates()
    {
        lock (_lock)
        {
            foreach (var instance in _instances.Values)
            {
                instance.PublishDspState();
            }
        }
    }

    internal RegistrySnapshotResult CreateSnapshot()
    {
        ManagedInstance[] instances;
        lock (_lock)
        {
            instances = _instances.Values
                .OrderBy(instance => instance.InstanceId.Value)
                .ToArray();
        }

        return new RegistrySnapshotResult(
            instances.Select(instance => new RegistryInstanceSnapshot(
                instance.InstanceId.Value,
                instance.State.Instance.Label.Value,
                instance.State.Instance.FocusedBank is { } focused
                    ? (ulong)focused.BankIndex
                    : null,
                instance.State.Instance.Banks
                    .Select(bank => new RegistryBankSnapshot(
                        (int)bank.Id,
                        bank.Group.Value?.Value))
                    .ToArray()))
            .ToArray());
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
            var instance = new InstanceState(
                instanceId,
                _stateValueFactory,
                runtime,
                _audibilityObserver,
                _topologyObserver);
            var dsp = new DspState(instanceId, _stateValueFactory, runtime);
            var root = _stateRegistry.GetRoot(instanceId);
            var state = new ManagedState(instance, dsp, runtime, root);
            _topologyObserver.AddState(state.Instance);
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




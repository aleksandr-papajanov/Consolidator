using Consolidator.Managed.Core.Abstractions;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Instances;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.Topology;
using Consolidator.Managed.Protocol;

namespace Consolidator.Managed.Core;

public sealed class Coordinator
{
    private readonly object _operationLock = new();
    private readonly IConsolidatorLogger _logger;
    private readonly StateHistory _history;
    private readonly DspStateCompiler _dspCompiler;
    private readonly InstanceStateBuilder _stateBuilder;
    private readonly InstanceRegistry _instanceRegistry;
    private readonly TopologyStore _topologyStore;
    private ulong _nextInstanceId;

    public Coordinator(
        IConsolidatorLogger logger,
        StateHistory history,
        DspStateCompiler dspCompiler,
        InstanceStateBuilder stateBuilder,
        InstanceRegistry instanceRegistry,
        TopologyStore topologyStore)
    {
        _logger = logger;
        _history = history;
        _dspCompiler = dspCompiler;
        _stateBuilder = stateBuilder;
        _instanceRegistry = instanceRegistry;
        _topologyStore = topologyStore;
    }

    public ConsolidatorInstance RegisterInstance(
        IInstanceOutput output,
        IDspStatePublisher dspPublisher)
    {
        ArgumentNullException.ThrowIfNull(output);
        ArgumentNullException.ThrowIfNull(dspPublisher);

        lock (_operationLock)
        {
            var id = ++_nextInstanceId;
            var instance = new ConsolidatorInstance(
                id,
                output,
                dspPublisher,
                _history,
                _dspCompiler,
                DspDefaults.CreateState(),
                _stateBuilder);
            _instanceRegistry.Add(instance);
            _topologyStore.Register(instance.Id);

            _logger.Info($"Registered instance {instance.Id}");

            return instance;
        }
    }

    public void UnregisterInstance(
        ulong instanceId)
    {
        lock (_operationLock)
        {
            var instance = _instanceRegistry.Remove(instanceId);
            if (instance is null)
            {
                return;
            }

            _topologyStore.Unregister(instanceId);
            instance.Stop();
            _logger.Info($"Unregistered instance {instanceId}");
        }
    }

    public void ReceiveMessage(
        ulong instanceId,
        string selector,
        ReadOnlySpan<Atom> atoms)
    {
        lock (_operationLock)
        {
        }
    }

    public void Prepare(
        ulong instanceId,
        double sampleRate,
        nuint maximumFrameCount)
    {
        lock (_operationLock)
        {
            var instance = _instanceRegistry.Find(instanceId);

            if (instance is null)
            {
                return;
            }

            instance.Prepare(
                sampleRate,
                maximumFrameCount);
        }
    }

    public TopologyState? GetTopology(ulong instanceId)
    {
        lock (_operationLock)
        {
            return _topologyStore.Get(instanceId);
        }
    }

    public bool SetBankGroup(
        ulong instanceId,
        int bankIndex,
        GroupId? groupId)
    {
        lock (_operationLock)
        {
            return _topologyStore.SetBankGroup(instanceId, bankIndex, groupId);
        }
    }

    public bool SetFocusedBank(
        ulong instanceId,
        BankAddress? focusedBank)
    {
        lock (_operationLock)
        {
            return _topologyStore.SetFocusedBank(instanceId, focusedBank);
        }
    }

    public IReadOnlyList<BankAddress> GetGroupMembers(BankAddress bank)
    {
        lock (_operationLock)
        {
            return _topologyStore.GetGroupMembers(bank);
        }
    }

    public ConsolidatorInstance? Resolve(BankAddress bank)
    {
        lock (_operationLock)
        {
            return _instanceRegistry.Find(bank.InstanceId);
        }
    }

    public IReadOnlyList<ConsolidatorInstance> GetGroupInstances(GroupId groupId)
    {
        lock (_operationLock)
        {
            return _topologyStore.GetGroupInstanceIds(groupId)
                .Select(_instanceRegistry.Find)
                .Where(instance => instance is not null)
                .Cast<ConsolidatorInstance>()
                .ToArray();
        }
    }

    public IReadOnlyList<ulong> GetGroupInstanceIds(GroupId groupId)
    {
        lock (_operationLock)
        {
            return _topologyStore.GetGroupInstanceIds(groupId);
        }
    }

    public IReadOnlyList<BankAddress> GetGroupedBanks(ulong instanceId)
    {
        lock (_operationLock)
        {
            return _topologyStore.GetGroupedBanks(instanceId);
        }
    }

    public IReadOnlyList<BankAddress> GetConnectedGroupBanks(
        IReadOnlyList<BankAddress> seeds)
    {
        lock (_operationLock)
        {
            return _topologyStore.GetConnectedGroupBanks(seeds);
        }
    }

    /// <summary>
    /// Opens a history point before the next logical operation begins.
    /// </summary>
    public void AdvanceHistoryPoint()
    {
        lock (_operationLock)
        {
            _history.AdvanceHistoryPoint();
        }
    }

    public bool UndoHistory()
    {
        lock (_operationLock)
        {
            if (!_history.Undo())
            {
                return false;
            }

            PublishDspStates();
            return true;
        }
    }

    public bool RedoHistory()
    {
        lock (_operationLock)
        {
            if (!_history.Redo())
            {
                return false;
            }

            PublishDspStates();
            return true;
        }
    }

    private void PublishDspStates()
    {
        var instances = _instanceRegistry.GetAll();

        foreach (var instance in instances)
        {
            instance.PublishDspState();
        }
    }

}
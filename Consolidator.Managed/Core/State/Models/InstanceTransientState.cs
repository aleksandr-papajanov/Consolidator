using Consolidator.Managed.Core.State.Observers;

namespace Consolidator.Managed.Core.State.Models;

public sealed class InstanceTransientState
{
    private readonly object _selectionLock = new();
    private readonly StateTopologyObserver _topologyObserver;
    private BankAddress? _focusedBank;

    internal InstanceTransientState(
        InstanceId instanceId,
        StateTopologyObserver topologyObserver)
    {
        InstanceId = instanceId;
        _topologyObserver = topologyObserver;
        _focusedBank = new BankAddress(instanceId, 0);
    }

    public InstanceId InstanceId { get; }

    public BankAddress? FocusedBank
    {
        get
        {
            lock (_selectionLock)
            {
                return _focusedBank;
            }
        }
        set
        {
            lock (_selectionLock)
            {
                if (_focusedBank == value)
                {
                    return;
                }
                _focusedBank = value;
            }

            _topologyObserver.FocusedBankChanged(InstanceId, value);
        }
    }

    public ProcessorId SnapshotContext { get; set; } = ProcessorId.Equalizer;
}

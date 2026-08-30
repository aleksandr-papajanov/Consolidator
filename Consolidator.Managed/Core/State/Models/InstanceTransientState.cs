using Consolidator.Managed.Core.State.Observers;

namespace Consolidator.Managed.Core.State.Models;

public sealed class InstanceTransientState
{
    private readonly object _selectionLock = new();
    private readonly StateTopologyObserver _topologyObserver;
    private InstanceSelectionContext _selection;

    internal InstanceTransientState(
        InstanceId instanceId,
        StateTopologyObserver topologyObserver)
    {
        InstanceId = instanceId;
        _topologyObserver = topologyObserver;
        _selection = new InstanceSelectionContext(
            new BankAddress(instanceId, 0),
            ProcessorId.Equalizer);
    }

    public InstanceId InstanceId { get; }

    public InstanceSelectionContext Selection
    {
        get
        {
            lock (_selectionLock)
            {
                return _selection;
            }
        }
    }

    public void SelectTarget(
        BankAddress selectedBank,
        ProcessorId selectedProcessor)
    {
        lock (_selectionLock)
        {
            if (_selection.SelectedBank == selectedBank &&
                _selection.SelectedProcessor == selectedProcessor)
            {
                return;
            }
            _selection = new InstanceSelectionContext(
                selectedBank,
                selectedProcessor);
        }

        _topologyObserver.FocusedBankChanged(InstanceId, selectedBank);
    }
}

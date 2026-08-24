using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.State.History;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Protocol.Notifications;

namespace Consolidator.Managed.Core.Services;

public sealed class HistoryNavigation : IHistoryNavigation
{
    private readonly StateHistory _history;
    private readonly InstanceRegistry _instanceRegistry;
    private readonly IOperationGate _operationGate;

    internal HistoryNavigation(
        StateHistory history,
        InstanceRegistry instanceRegistry,
        IOperationGate operationGate,
        HistoryStatePublisher historyStatePublisher)
    {
        _history = history;
        _instanceRegistry = instanceRegistry;
        _operationGate = operationGate;
    }

    public void AdvanceHistoryPoint()
    {
        using (_operationGate.Enter())
        {
            _history.AdvanceHistoryPoint();
        }
    }

    public bool JumpToHistory(int cursor)
    {
        using (_operationGate.Enter())
        {
            if (!_history.JumpToHistory(cursor))
            {
                return false;
            }

            PublishDspStates();
            return true;
        }
    }

    private void PublishDspStates()
    {
        _instanceRegistry.PublishDspStates();
    }
}





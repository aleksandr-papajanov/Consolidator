using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.State.History;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Protocol.Notifications;

namespace Consolidator.Managed.Core.Services;

public sealed class HistoryNavigation : IHistoryNavigation
{
    private readonly StateHistory _history;
    private readonly InstanceRegistry _instanceRegistry;

    internal HistoryNavigation(
        StateHistory history,
        InstanceRegistry instanceRegistry,
        HistoryStatePublisher historyStatePublisher)
    {
        _history = history;
        _instanceRegistry = instanceRegistry;
    }

    public void AdvanceHistoryPoint()
    {
        _history.AdvanceHistoryPoint();
    }

    public bool JumpToHistory(int cursor)
    {
        if (!_history.JumpToHistory(cursor))
        {
            return false;
        }

        PublishDspStates();
        return true;
    }

    private void PublishDspStates()
    {
        _instanceRegistry.PublishDspStates();
    }
}





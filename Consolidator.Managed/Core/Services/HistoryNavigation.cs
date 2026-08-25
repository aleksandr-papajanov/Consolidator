using Consolidator.Managed.Analyzer;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Protocol.Notifications;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Services;

public sealed class HistoryNavigation : IHistoryNavigation
{
    private readonly StateHistory _history;
    private readonly InstanceRegistry _instanceRegistry;
    private readonly AnalyzerRegistry _analyzerRegistry;
    private readonly DspStateChangeTracker _dspChanges;

    internal HistoryNavigation(
        StateHistory history,
        InstanceRegistry instanceRegistry,
        AnalyzerRegistry analyzerRegistry,
        DspStateChangeTracker dspChanges,
        HistoryStatePublisher historyStatePublisher)
    {
        _history = history;
        _instanceRegistry = instanceRegistry;
        _analyzerRegistry = analyzerRegistry;
        _dspChanges = dspChanges;
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

        _analyzerRegistry.CapturePendingInputs();
        _instanceRegistry.PublishDspStates(_dspChanges.Drain());
        return true;
    }
}


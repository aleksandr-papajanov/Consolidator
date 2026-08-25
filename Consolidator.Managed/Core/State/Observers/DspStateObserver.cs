using Consolidator.Managed.Analyzer;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed class DspStateObserver<TValue> : IStateValueObserver<TValue>
{
    private readonly AnalyzerRegistry _registry;
    private readonly DspStateChangeTracker _changes;
    private readonly InstanceId _instanceId;
    private readonly StatePath _path;

    public DspStateObserver(
        AnalyzerRegistry registry,
        DspStateChangeTracker changes,
        InstanceId instanceId,
        StatePath path)
    {
        ArgumentNullException.ThrowIfNull(registry);
        ArgumentNullException.ThrowIfNull(changes);
        ArgumentNullException.ThrowIfNull(path);

        _registry = registry;
        _changes = changes;
        _instanceId = instanceId;
        _path = path;
    }

    public void Attach(StateValue<TValue> value)
    {
    }

    public void ValueChanged(
        StateValue<TValue> value,
        TValue previousValue,
        TValue currentValue)
    {
        _changes.MarkChanged(_instanceId);
        _registry.UpdateFilter(_instanceId, _path);
    }

    public void Detach(StateValue<TValue> value)
    {
    }
}

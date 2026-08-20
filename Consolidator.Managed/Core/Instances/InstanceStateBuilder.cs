using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Bindings;

namespace Consolidator.Managed.Core.Instances;

public sealed class InstanceStateBuilder
{
    public InstanceStateStore Build(
        StateHistory history,
        DspState state)
    {
        ArgumentNullException.ThrowIfNull(history);
        ArgumentNullException.ThrowIfNull(state);

        var store = new InstanceStateStore();
        store.Add(history.CreateValue(
            StateIds.Gain,
            state.InputGain.GainDb.Value,
            new StateBinding<float>(value => state.InputGain.GainDb.SetValue(value))));
        return store;
    }
}

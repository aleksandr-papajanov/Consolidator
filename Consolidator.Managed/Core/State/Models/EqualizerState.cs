using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class EqualizerState
{
    public EqualizerState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        DspRuntimeState runtime)
    {
        Bypass = values.CreateValueWithoutHistory(
            instanceId,
            path.Append(StateNodeIds.Bypass),
            false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value =>
            {
                runtime.EqualizerBypass = value;
                runtime.EqualizerActive = !value;
            })]);
    }

    public StateValue<bool> Bypass { get; }
}







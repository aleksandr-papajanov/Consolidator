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
        Bypass = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Bypass),
            false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value =>
            {
                runtime.EqualizerBypass = value;
                runtime.EqualizerActive = !value;
            })]);
        Solo = values.CreateValue(instanceId, path.Append(StateNodeIds.Solo), false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value => runtime.EqualizerSolo = value)]);
    }

    public StateValue<bool> Bypass { get; }
    public StateValue<bool> Solo { get; }
}







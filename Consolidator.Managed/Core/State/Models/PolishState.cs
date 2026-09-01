using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class PolishState
{
    public PolishState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        DspRuntimeState runtime)
    {
        Thick = values.CreateValue(instanceId, path.Append(StateNodeIds.Thick), 0.0F,
            StateValueEditMode.ApplyDelta, DspParameterRanges.Macro,
            [new StateProjectionObserver<float>(value => runtime.PolishThick = value)]);
        Air = values.CreateValue(instanceId, path.Append(StateNodeIds.Air), 0.0F,
            StateValueEditMode.ApplyDelta, DspParameterRanges.Macro,
            [new StateProjectionObserver<float>(value => runtime.PolishAir = value)]);
        Bypass = values.CreateValueWithoutHistory(instanceId, path.Append(StateNodeIds.Bypass), false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value => runtime.PolishBypass = value)]);
    }

    public StateValue<float> Thick { get; }
    public StateValue<float> Air { get; }
    public StateValue<bool> Bypass { get; }
}

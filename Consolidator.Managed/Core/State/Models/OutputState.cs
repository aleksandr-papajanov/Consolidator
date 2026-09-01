using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class OutputState
{
    public OutputState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        DspRuntimeState runtime)
    {
        Level = values.CreateValue(instanceId, path.Append(StateNodeIds.Level), 0.0F,
            StateValueEditMode.ApplyDelta, DspParameterRanges.GainDb,
            [new StateProjectionObserver<float>(value => runtime.OutputLevel = value)]);
        Target = values.CreateValue(instanceId, path.Append(StateNodeIds.Target), -1.0F,
            StateValueEditMode.ApplyDelta, DspParameterRanges.TargetDb,
            [new StateProjectionObserver<float>(value => runtime.OutputTarget = value)]);
        Limiter = values.CreateValue(instanceId, path.Append(StateNodeIds.Limiter), false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value => runtime.OutputLimiter = value)]);
        Bypass = values.CreateValueWithoutHistory(instanceId, path.Append(StateNodeIds.Bypass), false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value => runtime.OutputGainBypass = value)]);
    }

    public StateValue<float> Level { get; }
    public StateValue<float> Target { get; }
    public StateValue<bool> Limiter { get; }
    public StateValue<bool> Bypass { get; }
}

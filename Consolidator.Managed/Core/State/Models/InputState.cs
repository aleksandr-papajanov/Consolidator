using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class InputState
{
    public InputState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        DspRuntimeState runtime)
    {
        Level = values.CreateValue(instanceId, path.Append(StateNodeIds.Level), 0.0F,
            StateValueEditMode.ApplyDelta, DspParameterRanges.GainDb,
            [new StateProjectionObserver<float>(value => runtime.InputLevel = value)]);
        Target = values.CreateValue(instanceId, path.Append(StateNodeIds.Target), -18.0F,
            StateValueEditMode.ApplyDelta, DspParameterRanges.TargetDb,
            [new StateProjectionObserver<float>(value => runtime.InputTarget = value)]);
        Width = values.CreateValue(instanceId, path.Append(StateNodeIds.Width), 100.0F,
            StateValueEditMode.ApplyDelta, DspParameterRanges.Width,
            [new StateProjectionObserver<float>(value => runtime.InputWidth = value)]);
        Leveler = values.CreateValue(instanceId, path.Append(StateNodeIds.Leveler), false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value => runtime.InputLeveler = value)]);
        Bypass = values.CreateValueWithoutHistory(instanceId, path.Append(StateNodeIds.Bypass), false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value => runtime.InputGainBypass = value)]);
        Detector = new DetectorState(
            instanceId,
            path.Append(StateNodeIds.Detector),
            values,
            (index, active) => runtime.SetDetectorFilterActive(index, active));
    }

    public StateValue<float> Level { get; }
    public StateValue<float> Target { get; }
    public StateValue<float> Width { get; }
    public StateValue<bool> Leveler { get; }
    public StateValue<bool> Bypass { get; }

    public DetectorState Detector { get; }
}

using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class SaturatorState
{
    public SaturatorState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        DspRuntimeState runtime)
    {
        Drive = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Drive),
            0.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.Drive,
            [new StateProjectionObserver<float>(value => runtime.SaturatorDrive = value)]);
        Curve = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Curve),
            0.5F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.Curve,
            [new StateProjectionObserver<float>(value => runtime.SaturatorCurve = value)]);
        Split = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Split),
            false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value => runtime.SaturatorSplit = value)]);
        OutputDb = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Output),
            0.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.OutputDb,
            [new StateProjectionObserver<float>(value => runtime.SaturatorOutputDb = value)]);
        Bypass = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Bypass),
            false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value =>
            {
                runtime.SaturatorBypass = value;
                runtime.SaturatorActive = !value;
            })]);
        Solo = values.CreateValue(instanceId, path.Append(StateNodeIds.Solo), false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value => runtime.SaturatorSolo = value)]);
        Detector = new DetectorState(
            instanceId,
            path.Append(StateNodeIds.Detector),
            values,
            value => runtime.SaturatorListen = value,
            (index, active) => runtime.SetDetectorFilterActive(
                DspConstants.DetectorFilterCount + index, active));
    }

    public StateValue<float> Drive { get; }

    public StateValue<float> Curve { get; }

    public StateValue<bool> Split { get; }

    public StateValue<float> OutputDb { get; }

    public StateValue<bool> Bypass { get; }
    public StateValue<bool> Solo { get; }

    public DetectorState Detector { get; }
}

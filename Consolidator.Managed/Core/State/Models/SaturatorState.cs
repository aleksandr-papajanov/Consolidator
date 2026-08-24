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
        OutputDb = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Output),
            0.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.OutputDb,
            [new StateProjectionObserver<float>(value => runtime.SaturatorOutputDb = value)]);
        Mix = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Mix),
            1.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.Mix,
            [new StateProjectionObserver<float>(value => runtime.SaturatorMix = value)]);
        DetectorAmount = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.DetectorAmount),
            1.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.DetectorAmount,
            [new StateProjectionObserver<float>(value => runtime.SaturatorDetectorAmount = value)]);
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
        Solo = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Solo),
            false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value => runtime.SaturatorSolo = value)]);
        Detector = new DetectorState(
            instanceId,
            path.Append(StateNodeIds.Detector),
            values,
            value => runtime.SaturatorListen = value,
            (index, active) => runtime.SetDetectorFilterActive(index, active));
    }

    public StateValue<float> Drive { get; }

    public StateValue<float> OutputDb { get; }

    public StateValue<float> Mix { get; }

    public StateValue<float> DetectorAmount { get; }

    public StateValue<bool> Bypass { get; }

    public StateValue<bool> Solo { get; }

    public DetectorState Detector { get; }
}

using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class CompressorState
{
    public CompressorState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        DspRuntimeState runtime)
    {
        ThresholdDb = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Threshold),
            -24.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.ThresholdDb,
            [new StateProjectionObserver<float>(value => runtime.CompressorThresholdDb = value)]);
        Ratio = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Ratio),
            4.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.Ratio,
            [new StateProjectionObserver<float>(value => runtime.CompressorRatio = value)]);
        AttackMs = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Attack),
            10.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.AttackMs,
            [new StateProjectionObserver<float>(value => runtime.CompressorAttackMs = value)]);
        ReleaseMs = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Release),
            100.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.ReleaseMs,
            [new StateProjectionObserver<float>(value => runtime.CompressorReleaseMs = value)]);
        OutputDb = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Output),
            0.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.OutputDb,
            [new StateProjectionObserver<float>(value => runtime.CompressorOutputDb = value)]);
        Mix = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Mix),
            1.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.Mix,
            [new StateProjectionObserver<float>(value => runtime.CompressorMix = value)]);
        Bypass = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Bypass),
            false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value =>
            {
                runtime.CompressorBypass = value;
                runtime.CompressorActive = !value;
            })]);
        Solo = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Solo),
            false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value => runtime.CompressorSolo = value)]);
        Detector = new DetectorState(
            instanceId,
            path.Append(StateNodeIds.Detector),
            values,
            value => runtime.CompressorListen = value,
            (index, active) => runtime.SetDetectorFilterActive(
                DspConstants.DetectorFilterCount + index,
                active));
    }

    public StateValue<float> ThresholdDb { get; }

    public StateValue<float> Ratio { get; }

    public StateValue<float> AttackMs { get; }

    public StateValue<float> ReleaseMs { get; }

    public StateValue<float> OutputDb { get; }

    public StateValue<float> Mix { get; }

    public StateValue<bool> Bypass { get; }

    public StateValue<bool> Solo { get; }

    public DetectorState Detector { get; }
}

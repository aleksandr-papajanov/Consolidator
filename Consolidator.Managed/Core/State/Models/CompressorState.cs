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
        Attack = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Attack),
            0.5F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.Macro,
            [new StateProjectionObserver<float>(value => runtime.CompressorAttack = value)]);
        Sustain = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Sustain),
            0.5F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.Macro,
            [new StateProjectionObserver<float>(value => runtime.CompressorSustain = value)]);
        Compression = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Compression),
            0.5F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.Macro,
            [new StateProjectionObserver<float>(value => runtime.CompressorCompression = value)]);
        Character = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Character),
            0,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<int>(value => runtime.CompressorCharacter = value)]);
        Parallel = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Parallel),
            false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(value => runtime.CompressorParallel = value)]);
        OutputDb = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Output),
            0.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.OutputDb,
            [new StateProjectionObserver<float>(value => runtime.CompressorOutputDb = value)]);
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
                DspConstants.DetectorFilterCount * 2 + index,
                active));
    }

    public StateValue<float> Attack { get; }

    public StateValue<float> Sustain { get; }

    public StateValue<float> Compression { get; }

    public StateValue<int> Character { get; }

    public StateValue<bool> Parallel { get; }

    public StateValue<float> OutputDb { get; }

    public StateValue<bool> Bypass { get; }

    public StateValue<bool> Solo { get; }

    public DetectorState Detector { get; }
}

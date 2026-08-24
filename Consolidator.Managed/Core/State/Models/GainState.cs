using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Models;

public sealed class GainState
{
    public GainState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        float gain,
        Action<float>? gainProjection = null,
        Action<bool>? bypassProjection = null)
    {
        IStateValueObserver<float>[] gainObservers = gainProjection is null
            ? Array.Empty<IStateValueObserver<float>>()
            : [new StateProjectionObserver<float>(gainProjection)];
        GainDb = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Gain),
            gain,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.GainDb,
            gainObservers);

        IStateValueObserver<bool>[] bypassObservers = bypassProjection is null
            ? Array.Empty<IStateValueObserver<bool>>()
            : [new StateProjectionObserver<bool>(bypassProjection)];
        Bypass = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Bypass),
            false,
            StateValueEditMode.CopyValue,
            observers: bypassObservers);
    }

    public StateValue<float> GainDb { get; }

    public StateValue<bool> Bypass { get; }
}

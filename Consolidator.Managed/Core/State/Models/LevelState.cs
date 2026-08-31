using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class LevelState
{
    public LevelState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        Action<float> levelProjection)
    {
        Level = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Level),
            0.0F,
            StateValueEditMode.ApplyDelta,
            DspParameterRanges.GainDb,
            [new StateProjectionObserver<float>(levelProjection)]);
    }

    public StateValue<float> Level { get; }
}

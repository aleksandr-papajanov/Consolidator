using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class DetectorState
{
    public DetectorState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        Action<bool> listenProjection,
        Action<int, bool> filterActiveProjection)
    {
        Listen = values.CreateValue(
            instanceId,
            path.Append(StateNodeIds.Listen),
            false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(listenProjection)]);
        Filters = Enumerable.Range(0, DspConstants.DetectorFilterCount)
            .Select(index => new FilterState(
                instanceId,
                path.Append(StateNodeIds.Filter).Append(StateNodeIds.FilterAt(index)),
                values,
                false,
                bypass => filterActiveProjection(index, !bypass)))
            .ToArray();
    }

    public StateValue<bool> Listen { get; }
    public FilterState[] Filters { get; }

}







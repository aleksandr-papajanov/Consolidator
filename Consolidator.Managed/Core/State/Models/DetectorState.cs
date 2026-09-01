using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class DetectorState
{
    public DetectorState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        Action<int, bool> filterActiveProjection)
    {
        Filters = Enumerable.Range(0, DspConstants.DetectorFilterCount)
            .Select(index => new FilterState(
                instanceId,
                path.Append(StateNodeIds.Filter).Append(StateNodeIds.FilterAt(index)),
                values,
                false,
                bypass => filterActiveProjection(index, !bypass),
                FilterCatalog.Detector[index]))
            .ToArray();
    }

    public FilterState[] Filters { get; }

}







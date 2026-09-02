using Consolidator.Managed.Core.Dsp;

namespace Consolidator.Managed.Core.State.Models;

internal sealed class StateModelContext
{
    public StateModelContext(
        InstanceId instanceId,
        StateValueFactory values,
        DspRuntimeState runtime)
    {
        ArgumentNullException.ThrowIfNull(values);
        ArgumentNullException.ThrowIfNull(runtime);

        InstanceId = instanceId;
        Values = values;
        Runtime = runtime;
    }

    public InstanceId InstanceId { get; }

    public StateValueFactory Values { get; }

    public DspRuntimeState Runtime { get; }
}
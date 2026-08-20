using Consolidator.Managed.Core.State.Bindings;

namespace Consolidator.Managed.Core.Topology;

internal sealed class TopologyBinding : IStateBinding<TopologyState>
{
    private readonly ulong _instanceId;
    private readonly TopologyIndex _index;

    public TopologyBinding(
        ulong instanceId,
        TopologyIndex index)
    {
        _instanceId = instanceId;
        _index = index;
    }

    public void Apply(TopologyState value)
    {
        _index.Set(_instanceId, value);
    }
}

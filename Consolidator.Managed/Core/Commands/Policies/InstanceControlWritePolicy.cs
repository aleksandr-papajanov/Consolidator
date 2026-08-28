using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Commands.Policies;

internal sealed class InstanceControlWritePolicy : IStateWritePolicy
{
    public bool Applies(StatePath path)
    {
        var nodes = path.Nodes;
        return nodes.Count == 2 &&
            nodes[0] == StateNodeIds.Instance &&
            (nodes[1] == StateNodeIds.Mute ||
                nodes[1] == StateNodeIds.Solo);
    }

    public bool IsAllowed(
        WriteStateCommand command,
        InstanceCommandContext context)
    {
        return false;
    }
}

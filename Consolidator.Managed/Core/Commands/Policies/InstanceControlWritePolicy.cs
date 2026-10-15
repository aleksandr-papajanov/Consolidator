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
        if (nodes.Count == 2 &&
            nodes[0] == StateNodeIds.Instance &&
            (nodes[1] == StateNodeIds.Mute ||
                nodes[1] == StateNodeIds.Solo ||
                nodes[1] == StateNodeIds.Bypass))
        {
            return true;
        }

        return nodes.Count == 3 &&
            nodes[0] == StateNodeIds.Dsp &&
            (nodes[1] == StateNodeIds.GainStaging ||
                nodes[1] == StateNodeIds.Saturator ||
                nodes[1] == StateNodeIds.Shaper ||
                nodes[1] == StateNodeIds.Compression ||
                nodes[1] == StateNodeIds.Equalizer) &&
            (nodes[2] == StateNodeIds.Bypass ||
                nodes[2] == StateNodeIds.Solo);
    }

    public bool IsAllowed(
        WriteStateCommand command,
        InstanceCommandContext context)
    {
        return false;
    }
}

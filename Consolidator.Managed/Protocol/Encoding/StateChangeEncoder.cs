using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Protocol.Encoding;

internal static class StateChangeEncoder
{
    public static ProtocolOutput Encode(
        StateValueChanged change,
        IReadOnlyList<ulong> targetInstanceIds)
    {
        ArgumentNullException.ThrowIfNull(change);
        ArgumentNullException.ThrowIfNull(targetInstanceIds);

        return new ProtocolOutput(
            targetInstanceIds,
            "state_changed",
            [
                new Atom(AtomType.Symbol, 0, 0, FormatPath(change.Path)),
                ProtocolAtomEncoder.EncodeValue(change.PreviousValue),
                ProtocolAtomEncoder.EncodeValue(change.CurrentValue)
            ]);
    }

    private static string FormatPath(StatePath path)
    {
        return string.Join(
            ".",
            path.Nodes.Select(node => node.Value));
    }

}

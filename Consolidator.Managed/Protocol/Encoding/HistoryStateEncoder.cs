using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Protocol.Encoding;

internal static class HistoryStateEncoder
{
    public static ProtocolOutput Encode(
        StateHistorySnapshot snapshot,
        IReadOnlyList<ulong> targetInstanceIds)
    {
        ArgumentNullException.ThrowIfNull(targetInstanceIds);

        return new ProtocolOutput(
            targetInstanceIds,
            "history_state",
            [
                new Atom(AtomType.Integer, (long)snapshot.Revision, 0, null),
                new Atom(AtomType.Integer, snapshot.Cursor, 0, null),
                new Atom(AtomType.Integer, snapshot.EntryCount, 0, null),
                new Atom(AtomType.Integer, snapshot.CanUndo ? 1 : 0, 0, null),
                new Atom(AtomType.Integer, snapshot.CanRedo ? 1 : 0, 0, null)
            ]);
    }
}
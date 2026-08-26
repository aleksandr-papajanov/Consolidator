using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Protocol.Encoding;

internal static class StateChangeEncoder
{
    public static ProtocolOutput Encode(
        StateValueChanged change,
        IReadOnlyList<ulong> targetInstanceIds,
        StateValueMetadata metadata,
        FloatRange? effectiveRange,
        int? bankId)
    {
        ArgumentNullException.ThrowIfNull(change);
        ArgumentNullException.ThrowIfNull(targetInstanceIds);

        var path = StatePathEncoder.Encode(change.Path, bankId);
        return new ProtocolOutput(
            targetInstanceIds,
            "state_changed",
            [
                new Atom(AtomType.Integer, 1, 0, null),
                new Atom(AtomType.Symbol, 0, 0, path),
                ProtocolAtomEncoder.EncodeValue(change.CurrentValue),
                new Atom(AtomType.Symbol, 0, 0, "ready"),
                Optional(metadata.PhysicalRange?.Minimum),
                Optional(metadata.PhysicalRange?.Maximum),
                Optional(effectiveRange?.Minimum),
                Optional(effectiveRange?.Maximum)
            ],
            DeliverySemantics.ActivePresentation);
    }

    private static Atom Optional(float? value) => value is { } number
        ? new Atom(AtomType.Float, 0, number, null)
        : new Atom(AtomType.Symbol, 0, 0, "none");

}

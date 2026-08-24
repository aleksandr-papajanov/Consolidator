using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Decoding;

internal static class CommandCodecSupport
{
    public static CommandFrameHeader DecodeHeader(
        string selector,
        ReadOnlySpan<Atom> atoms,
        ulong sourceInstanceId,
        ulong requestId)
    {
        const long protocolVersion = 1;
        if (atoms.Length < 3 ||
            atoms[0].Type != AtomType.Integer ||
            atoms[0].Integer != protocolVersion ||
            atoms[1].Type != AtomType.Symbol ||
            string.IsNullOrEmpty(atoms[1].Symbol) ||
            atoms[2].Type != AtomType.Symbol ||
            !ulong.TryParse(atoms[2].Symbol, out var wireRequestId))
        {
            throw new FormatException("Invalid protocol frame header.");
        }

        return new CommandFrameHeader(
            sourceInstanceId,
            wireRequestId == 0 ? requestId : wireRequestId,
            selector,
            3);
    }

    public static DecodedCommand Success(
        CommandFrameHeader header,
        object command)
    {
        return new DecodedCommand(
            header.SourceInstanceId,
            header.RequestId,
            header.Selector,
            command);
    }

    public static int ReadCount(Atom atom)
    {
        if (atom.Type != AtomType.Integer || atom.Integer is < 0 or > 16)
        {
            throw new FormatException("Command entry count is out of range.");
        }

        return (int)atom.Integer;
    }

    public static ulong ReadWireId(Atom atom)
    {
        if (atom.Type != AtomType.Symbol ||
            !ulong.TryParse(atom.Symbol, out var value))
        {
            throw new FormatException("Wire id must be an unsigned decimal symbol.");
        }

        return value;
    }

    public static bool IsSymbol(Atom atom, string value) =>
        atom.Type == AtomType.Symbol && atom.Symbol == value;

    public static void RequireInteger(
        ReadOnlySpan<Atom> atoms,
        int position,
        string name)
    {
        if (position >= atoms.Length || atoms[position].Type != AtomType.Integer)
        {
            throw new FormatException($"Missing {name}.");
        }
    }
}

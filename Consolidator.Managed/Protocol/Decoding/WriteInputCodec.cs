using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Protocol.Decoding;

internal sealed class WriteInputCodec : IInputCodec
{
    private readonly IStatePathDecoder _pathDecoder;

    public WriteInputCodec(IStatePathDecoder pathDecoder)
    {
        _pathDecoder = pathDecoder;
    }

    public string Selector => "write";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        if (atoms.Length < header.Position + 4)
        {
            throw new FormatException("Invalid write frame.");
        }

        var epoch = CommandCodecSupport.ReadWireId(atoms[header.Position]);
        var transactionId = CommandCodecSupport.ReadWireId(atoms[header.Position + 1]);
        var count = CommandCodecSupport.ReadCount(atoms[header.Position + 2]);
        var position = header.Position + 3;
        if (count != 1 || position >= atoms.Length ||
            !CommandCodecSupport.IsSymbol(atoms[position], "entry"))
        {
            throw new FormatException("Only one write entry is supported.");
        }

        position++;
        var valuePosition = FindValueMarker(atoms, position);
        var path = _pathDecoder.Decode(atoms[position..valuePosition]);
        var valuePositionAfterMarker = valuePosition + 1;
        if (valuePositionAfterMarker >= atoms.Length)
        {
            throw new FormatException("Write entry has no value.");
        }

        var value = DecodeValue(atoms[valuePositionAfterMarker], path);
        if (valuePositionAfterMarker + 1 != atoms.Length)
        {
            throw new FormatException("Write frame contains extra atoms.");
        }

        return CommandCodecSupport.Success(
            header,
            new WriteStateCommand(
                path,
                value.Value,
                value.ValueType,
                epoch,
                transactionId));
    }

    private static int FindValueMarker(ReadOnlySpan<Atom> atoms, int position)
    {
        for (var index = position; index < atoms.Length; index++)
        {
            if (CommandCodecSupport.IsSymbol(atoms[index], "value"))
            {
                return index;
            }
        }

        throw new FormatException("Write entry has no value marker.");
    }

    private static DecodedValue DecodeValue(Atom atom, StatePath path)
    {
        ArgumentNullException.ThrowIfNull(path);
        if (path.Depth == 0)
        {
            throw new FormatException("State value path is empty.");
        }

        var node = path.Nodes[^1];
        if (node == StateNodeIds.Label)
        {
            return new DecodedValue(ReadSymbol(atom), typeof(string));
        }

        if (node == StateNodeIds.Mute
            || node == StateNodeIds.Solo
            || node == StateNodeIds.Bypass
            || node == StateNodeIds.Listen)
        {
            return new DecodedValue(ReadBoolean(atom), typeof(bool));
        }

        if (node == StateNodeIds.Group)
        {
            return new DecodedValue(ReadGroup(atom), typeof(GroupId?));
        }

        if (node == StateNodeIds.FocusedBank)
        {
            throw new FormatException(
                "Focused bank values require source instance context.");
        }

        return new DecodedValue(ReadFloat(atom), typeof(float));
    }

    private static string ReadSymbol(Atom atom)
    {
        if (atom.Type != AtomType.Symbol || atom.Symbol is null)
        {
            throw new FormatException("Expected a symbol state value.");
        }

        return atom.Symbol;
    }

    private static bool ReadBoolean(Atom atom)
    {
        if (atom.Type != AtomType.Integer || atom.Integer is not (0 or 1))
        {
            throw new FormatException("Expected a boolean state value.");
        }

        return atom.Integer == 1;
    }

    private static GroupId? ReadGroup(Atom atom)
    {
        if (atom.Type == AtomType.Symbol && atom.Symbol == "none")
        {
            return null;
        }

        if (atom.Type != AtomType.Integer || atom.Integer < 0)
        {
            throw new FormatException("Expected a group id state value.");
        }

        return new GroupId((uint)atom.Integer);
    }

    private static float ReadFloat(Atom atom)
    {
        return atom.Type switch
        {
            AtomType.Integer => atom.Integer,
            AtomType.Float => (float)atom.Float,
            _ => throw new FormatException("Expected a numeric state value.")
        };
    }

    private readonly record struct DecodedValue(
        object? Value,
        Type ValueType);
}

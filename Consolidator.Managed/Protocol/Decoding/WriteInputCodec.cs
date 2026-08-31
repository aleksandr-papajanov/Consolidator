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
        if (atoms.Length < header.Position + 4 ||
            atoms[header.Position].Type != AtomType.Symbol)
        {
            throw new FormatException("Invalid write frame.");
        }

        var scope = atoms[header.Position].Symbol switch
        {
            "local" => WriteScope.Local,
            "group" => WriteScope.Group,
            "topology" => WriteScope.Topology,
            _ => throw new FormatException("Invalid write scope.")
        };
        var position = header.Position + 1;
        InstanceId? targetInstanceId = null;
        if (scope is WriteScope.Topology)
        {
            targetInstanceId = new InstanceId(
                CommandCodecSupport.ReadWireId(atoms[position++]));
        }
        var transactionId = CommandCodecSupport.ReadWireId(atoms[position++]);
        var count = CommandCodecSupport.ReadCount(atoms[position++]);
        var entries = new List<StateWriteEntry>(count);
        for (var index = 0; index < count; index++)
        {
            if (position >= atoms.Length ||
                !CommandCodecSupport.IsSymbol(atoms[position++], "entry"))
            {
                throw new FormatException("Write entry marker is missing.");
            }

            var valuePosition = FindValueMarker(atoms, position);
            var path = _pathDecoder.Decode(atoms[position..valuePosition]);
            position = valuePosition + 1;
            if (position >= atoms.Length)
            {
                throw new FormatException("Write entry has no value.");
            }

            var value = DecodeValue(atoms[position++], path);
            entries.Add(new StateWriteEntry(path, value.Value, value.ValueType));
        }

        if (position != atoms.Length)
        {
            throw new FormatException("Write frame contains extra atoms.");
        }

        return CommandCodecSupport.Success(
            header,
            new WriteStateCommand(
                entries,
                transactionId,
                scope,
                targetInstanceId));
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
            || node == StateNodeIds.Listen
            || node == StateNodeIds.Leveler
            || node == StateNodeIds.Split
            || node == StateNodeIds.Parallel
            || node == StateNodeIds.Limiter)
        {
            return new DecodedValue(ReadBoolean(atom), typeof(bool));
        }

        if (node == StateNodeIds.Group)
        {
            return new DecodedValue(ReadGroup(atom), typeof(GroupId?));
        }

        if (node == StateNodeIds.Character)
        {
            if (atom.Type != AtomType.Integer || atom.Integer is < 0 or > 2)
            {
                throw new FormatException("Expected a compressor character value.");
            }

            return new DecodedValue((int)atom.Integer, typeof(int));
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

        if (atom.Type != AtomType.Integer ||
            atom.Integer < 0 ||
            atom.Integer > uint.MaxValue)
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

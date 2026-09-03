using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Decoding;

internal sealed class ResetInputCodec : IInputCodec
{
    private readonly IStatePathDecoder _pathDecoder;

    public ResetInputCodec(IStatePathDecoder pathDecoder)
    {
        _pathDecoder = pathDecoder;
    }

    public string Selector => "reset";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        if (atoms.Length < header.Position + 2)
        {
            throw new FormatException("Invalid reset frame.");
        }

        var position = header.Position;
        InstanceId? targetInstanceId = null;
        int? bankIndex = null;
        if (atoms[position].Type == AtomType.Symbol && atoms[position].Symbol == "target")
        {
            if (atoms.Length < position + 5)
            {
                throw new FormatException("Invalid targeted reset frame.");
            }

            targetInstanceId = new InstanceId(CommandCodecSupport.ReadWireId(atoms[position + 1]));
            if (atoms[position + 2].Type == AtomType.Symbol && atoms[position + 2].Symbol == "none")
            {
                bankIndex = null;
            }
            else if (atoms[position + 2].Type == AtomType.Integer)
            {
                bankIndex = checked((int)atoms[position + 2].Integer);
            }
            else
            {
                throw new FormatException("Bank index must be an integer or none.");
            }
            position += 3;
        }

        var transactionId = CommandCodecSupport.ReadWireId(atoms[position]);
        var scope = atoms[position + 1].Type == AtomType.Symbol
            ? atoms[position + 1].Symbol switch
            {
                "local" => ResetScope.Local,
                "group" => ResetScope.Group,
                "group_instance" => ResetScope.GroupInstance,
                _ => throw new FormatException("Invalid reset scope.")
            }
            : throw new FormatException("Reset scope must be a symbol.");
        var path = _pathDecoder.Decode(
            atoms[(position + 2)..],
            allowContainer: true);
        return CommandCodecSupport.Success(
            header,
            new ResetStateCommand(
                path,
                transactionId,
                scope,
                targetInstanceId,
                bankIndex));
    }
}

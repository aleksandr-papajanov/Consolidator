using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.State;
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

        var transactionId = CommandCodecSupport.ReadWireId(atoms[header.Position]);
        var scope = atoms[header.Position + 1].Type == AtomType.Symbol
            ? atoms[header.Position + 1].Symbol switch
            {
                "local" => ResetScope.Local,
                "group" => ResetScope.Group,
                "group_instance" => ResetScope.GroupInstance,
                _ => throw new FormatException("Invalid reset scope.")
            }
            : throw new FormatException("Reset scope must be a symbol.");
        var path = _pathDecoder.Decode(
            atoms[(header.Position + 2)..],
            allowContainer: true);
        return CommandCodecSupport.Success(
            header,
            new ResetStateCommand(
                path,
                transactionId,
                scope));
    }
}

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

        var epoch = CommandCodecSupport.ReadWireId(atoms[header.Position]);
        var transactionId = CommandCodecSupport.ReadWireId(atoms[header.Position + 1]);
        var path = _pathDecoder.Decode(
            atoms[(header.Position + 2)..],
            allowContainer: true);
        return CommandCodecSupport.Success(
            header,
            new ResetStateCommand(path, epoch, transactionId));
    }
}

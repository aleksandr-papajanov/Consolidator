using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Decoding;

internal sealed class ReadInputCodec : IInputCodec
{
    private readonly IStatePathDecoder _pathDecoder;

    public ReadInputCodec(IStatePathDecoder pathDecoder)
    {
        _pathDecoder = pathDecoder;
    }

    public string Selector => "read";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        CommandCodecSupport.RequireInteger(atoms, header.Position, "read count");
        var count = CommandCodecSupport.ReadCount(atoms[header.Position]);
        var position = header.Position + 1;
        if (count != 1 || position >= atoms.Length ||
            !CommandCodecSupport.IsSymbol(atoms[position], "query"))
        {
            throw new FormatException("Only one read query is supported.");
        }

        var path = _pathDecoder.Decode(atoms[(position + 1)..]);
        return CommandCodecSupport.Success(header, new ReadStateCommand(path));
    }
}

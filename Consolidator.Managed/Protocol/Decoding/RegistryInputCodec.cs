using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Decoding;

internal sealed class RegistryInputCodec : IInputCodec
{
    public string Selector => "registry";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        if (atoms.Length != header.Position)
        {
            throw new FormatException("Invalid registry frame.");
        }

        return CommandCodecSupport.Success(header, new ReadRegistryCommand());
    }
}

using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Protocol.Decoding;

internal sealed class HistoryInputCodec : IInputCodec
{
    public string Selector => "jump_history";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        if (atoms.Length != header.Position + 1 ||
            atoms[header.Position].Type != AtomType.Integer ||
            atoms[header.Position].Integer < 0 ||
            atoms[header.Position].Integer > StateHistory.Capacity - 1)
        {
            throw new FormatException("Invalid history frame.");
        }

        return CommandCodecSupport.Success(
            header,
            new JumpToHistoryCommand((int)atoms[header.Position].Integer));
    }
}

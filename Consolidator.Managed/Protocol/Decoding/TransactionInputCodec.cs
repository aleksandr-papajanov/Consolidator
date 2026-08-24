using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Decoding;

internal sealed class TransactionInputCodec : IInputCodec
{
    private readonly bool _begin;

    public TransactionInputCodec(bool begin)
    {
        _begin = begin;
    }

    public string Selector => _begin ? "begin_history" : "end_history";

    public DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header)
    {
        if (atoms.Length != header.Position + 1)
        {
            throw new FormatException("Invalid transaction frame.");
        }

        var historyId = CommandCodecSupport.ReadWireId(atoms[header.Position]);
        object command = _begin
            ? new BeginHistoryCommand(historyId)
            : new EndHistoryCommand(historyId);
        return CommandCodecSupport.Success(header, command);
    }
}

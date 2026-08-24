using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Decoding;

internal interface IInputCodec
{
    string Selector { get; }

    DecodedCommand Decode(
        ReadOnlySpan<Atom> atoms,
        CommandFrameHeader header);
}

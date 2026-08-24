using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Protocol.Decoding;

internal interface IStatePathDecoder
{
    StatePath Decode(
        ReadOnlySpan<Atom> atoms,
        bool allowContainer = false);
}

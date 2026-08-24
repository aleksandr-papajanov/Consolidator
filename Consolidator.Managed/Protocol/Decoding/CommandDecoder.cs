using Consolidator.Managed.Protocol.Dispatch;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Decoding;

internal sealed class CommandDecoder
{
    private readonly IReadOnlyDictionary<string, IInputCodec> _codecs;

    public CommandDecoder(IEnumerable<IInputCodec> codecs)
    {
        ArgumentNullException.ThrowIfNull(codecs);

        var codecMap = new Dictionary<string, IInputCodec>(StringComparer.Ordinal);
        foreach (var codec in codecs)
        {
            if (!codecMap.TryAdd(codec.Selector, codec))
            {
                throw new InvalidOperationException(
                    $"Multiple input codecs are registered for selector '{codec.Selector}'.");
            }
        }

        _codecs = codecMap;
    }

    public DecodedCommand Decode(ProtocolInput message)
    {
        ArgumentNullException.ThrowIfNull(message);

        var atoms = message.Atoms.ToArray();
        var header = CommandCodecSupport.DecodeHeader(
            message.Selector,
            atoms,
            message.SourceInstanceId,
            0);
        if (!_codecs.TryGetValue(message.Selector, out var codec))
        {
            throw new KeyNotFoundException(
                $"Unknown command selector '{message.Selector}'.");
        }

        return codec.Decode(atoms, header);
    }
}

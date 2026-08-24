using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Encoding;

internal static class ProtocolErrorEncoder
{
    public static ProtocolOutput Encode(
        ulong sourceInstanceId,
        Exception exception)
    {
        ArgumentNullException.ThrowIfNull(exception);

        var code = exception switch
        {
            FormatException => "malformed",
            KeyNotFoundException => "unknown_selector",
            _ => "execution_failed"
        };

        return new ProtocolOutput(
            [sourceInstanceId],
            "error",
            [
                new Atom(AtomType.Integer, 1, 0, null),
                new Atom(AtomType.Symbol, 0, 0, sourceInstanceId.ToString()),
                new Atom(AtomType.Symbol, 0, 0, "0"),
                new Atom(AtomType.Symbol, 0, 0, code),
                new Atom(AtomType.Symbol, 0, 0, exception.Message)
            ]);
    }
}

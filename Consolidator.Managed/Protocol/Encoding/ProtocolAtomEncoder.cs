using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Encoding;

internal static class ProtocolAtomEncoder
{
    public static Atom EncodeValue(object? value)
    {
        return value switch
        {
            null => new Atom(AtomType.Symbol, 0, 0, string.Empty),
            bool boolean => new Atom(AtomType.Integer, boolean ? 1 : 0, 0, null),
            int integer => new Atom(AtomType.Integer, integer, 0, null),
            long integer => new Atom(AtomType.Integer, integer, 0, null),
            float number => new Atom(AtomType.Float, 0, number, null),
            double number => new Atom(AtomType.Float, 0, number, null),
            string text => new Atom(AtomType.Symbol, 0, 0, text),
            _ => new Atom(AtomType.Symbol, 0, 0, value.ToString())
        };
    }
}

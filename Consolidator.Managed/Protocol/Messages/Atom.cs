namespace Consolidator.Managed.Protocol.Messages;

public readonly record struct Atom(
    AtomType Type,
    long Integer,
    double Float,
    string? Symbol);




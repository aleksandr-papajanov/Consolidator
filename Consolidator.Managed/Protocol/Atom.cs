namespace Consolidator.Managed.Protocol;

public enum AtomType
{
    Integer,
    Float,
    Symbol
}

public readonly record struct Atom(
    AtomType Type,
    long Integer,
    double Float,
    string? Symbol);
namespace Consolidator.Managed.Protocol.Messages;

internal sealed record ProtocolOutput(
    IReadOnlyList<ulong> TargetInstanceIds,
    string Selector,
    IReadOnlyList<Atom> Atoms);

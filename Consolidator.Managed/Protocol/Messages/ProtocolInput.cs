namespace Consolidator.Managed.Protocol.Messages;

internal sealed record ProtocolInput(
    ulong SourceInstanceId,
    string Selector,
    IReadOnlyList<Atom> Atoms);

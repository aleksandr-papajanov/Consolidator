namespace Consolidator.Managed.Protocol.Messages;

internal enum DeliverySemantics
{
    Lossless,
    ActivePresentation,
    LatestAnalysis
}

internal sealed record ProtocolOutput(
    IReadOnlyList<ulong> TargetInstanceIds,
    string Selector,
    IReadOnlyList<Atom> Atoms,
    DeliverySemantics DeliverySemantics = DeliverySemantics.Lossless);

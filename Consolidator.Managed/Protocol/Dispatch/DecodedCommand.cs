namespace Consolidator.Managed.Protocol.Dispatch;

internal sealed record DecodedCommand(
    ulong SourceInstanceId,
    ulong RequestId,
    string Selector,
    object Command);




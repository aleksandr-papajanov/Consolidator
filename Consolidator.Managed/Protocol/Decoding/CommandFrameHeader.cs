namespace Consolidator.Managed.Protocol.Decoding;

internal readonly record struct CommandFrameHeader(
    ulong SourceInstanceId,
    ulong RequestId,
    string Selector,
    int Position);
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Protocol.Dispatch;

internal interface ICommandEndpoint
{
    string Selector { get; }

    Type MessageType { get; }

    ValueTask<ProtocolOutput> ExecuteAsync(
        ulong sourceInstanceId,
        object message,
        ulong requestId,
        CancellationToken cancellationToken = default);
}

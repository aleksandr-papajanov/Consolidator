using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Encoding;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Routing.Commands;

namespace Consolidator.Managed.Protocol.Dispatch;

internal sealed class CommandEndpoint<TCommand, TResult>
    : ICommandEndpoint
    where TCommand : IInstanceCommand<TResult>
{
    private readonly InstanceCommandRouter _router;
    private readonly CommandResponseEncoder _responseEncoder;
    private readonly string _selector;
    private readonly string _responseSelector;

    public CommandEndpoint(
        InstanceCommandRouter router,
        CommandResponseEncoder responseEncoder,
        string selector,
        string responseSelector)
    {
        ArgumentNullException.ThrowIfNull(router);
        ArgumentNullException.ThrowIfNull(responseEncoder);
        ArgumentException.ThrowIfNullOrEmpty(selector);
        ArgumentException.ThrowIfNullOrEmpty(responseSelector);

        _router = router;
        _responseEncoder = responseEncoder;
        _selector = selector;
        _responseSelector = responseSelector;
    }

    public string Selector => _selector;

    public Type MessageType => typeof(TCommand);

    public async ValueTask<IReadOnlyList<ProtocolOutput>> ExecuteAsync(
        ulong sourceInstanceId,
        object command,
        ulong requestId,
        CancellationToken cancellationToken = default)
    {
        if (command is not TCommand typedCommand)
        {
            throw new ArgumentException(
                $"Command type does not match endpoint: {command.GetType()}.",
                nameof(command));
        }

        var routed = await _router.ExecuteAsync(
            new InstanceId(sourceInstanceId),
            typedCommand,
            cancellationToken);
        return _responseEncoder.Encode(
            _responseSelector,
            routed.Execution,
            sourceInstanceId,
            requestId);
    }
}




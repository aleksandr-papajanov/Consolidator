using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Execution;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.Topology;

namespace Consolidator.Managed.Routing.Commands;

public sealed class InstanceCommandRouter
{
    private readonly InstanceRegistry _instanceRegistry;
    private readonly TopologyIndex _topologyIndex;
    private readonly CommandExecutor _executor;
    private readonly IOperationGate _operationGate;

    internal InstanceCommandRouter(
        InstanceRegistry instanceRegistry,
        TopologyIndex topologyIndex,
        CommandExecutor executor,
        IOperationGate operationGate)
    {
        _instanceRegistry = instanceRegistry;
        _topologyIndex = topologyIndex;
        _executor = executor;
        _operationGate = operationGate;
    }

    public async ValueTask<CommandRoutingResult<TResult>> ExecuteAsync<TResult>(
        InstanceId sourceInstanceId,
        IInstanceCommand<TResult> command,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(command);
        cancellationToken.ThrowIfCancellationRequested();

        using (await _operationGate.EnterAsync(cancellationToken))
        {
            if (!_instanceRegistry.Contains(sourceInstanceId))
            {
                return new CommandRoutingResult<TResult>(
                    Array.Empty<InstanceId>(),
                    CommandExecutionResult<TResult>.Failure(
                        0,
                        0,
                        $"Source instance was not found: {sourceInstanceId}."));
            }

            var targetInstanceIds = command.Scope is CommandScope.Coordinator
                ? [sourceInstanceId]
                : ResolveTargets(sourceInstanceId, command.Scope);
            var result = await _executor.ExecuteAsync(
                targetInstanceIds,
                command,
                cancellationToken);
            return new CommandRoutingResult<TResult>(targetInstanceIds, result);
        }
    }

    private IReadOnlyList<InstanceId> ResolveTargets(
        InstanceId sourceInstanceId,
        CommandScope scope)
    {
        return scope switch
        {
            CommandScope.FocusedBank => ResolveFocusedTarget(sourceInstanceId),
            CommandScope.ConnectedInstances =>
                _topologyIndex.ResolveConnectedInstanceIds(sourceInstanceId),
            CommandScope.Coordinator => Array.Empty<InstanceId>(),
            _ => throw new ArgumentOutOfRangeException(nameof(scope))
        };
    }

    private IReadOnlyList<InstanceId> ResolveFocusedTarget(InstanceId sourceInstanceId)
    {
        var instanceId = _topologyIndex.ResolveFocusedInstanceId(sourceInstanceId);
        return instanceId is { } value
            ? [value]
            : Array.Empty<InstanceId>();
    }
}

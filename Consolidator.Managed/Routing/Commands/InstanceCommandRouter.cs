using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Execution;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.Core.Topology;

namespace Consolidator.Managed.Routing.Commands;

public sealed class InstanceCommandRouter
{
    private readonly InstanceRegistry _instanceRegistry;
    private readonly TopologyIndex _topologyIndex;
    private readonly CommandExecutor _executor;
    private readonly IOperationGate _operationGate;
    private readonly StatePeerObserver _peerObserver;

    internal InstanceCommandRouter(
        InstanceRegistry instanceRegistry,
        TopologyIndex topologyIndex,
        CommandExecutor executor,
        IOperationGate operationGate,
        StatePeerObserver peerObserver)
    {
        _instanceRegistry = instanceRegistry;
        _topologyIndex = topologyIndex;
        _executor = executor;
        _operationGate = operationGate;
        _peerObserver = peerObserver;
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

            var targetInstanceIds = command is ITargetedInstanceCommand
                { TargetInstanceId: { } explicitTarget }
                ? [explicitTarget]
                : command.Scope is CommandScope.Coordinator
                    ? [sourceInstanceId]
                    : ResolveTargets(sourceInstanceId, command.Scope);
            _peerObserver.BeginEdit(
                _topologyIndex.ResolveFocusedBankAddress(sourceInstanceId));
            try
            {
                var result = await _executor.ExecuteAsync(
                    targetInstanceIds,
                    command,
                    cancellationToken);
                return new CommandRoutingResult<TResult>(targetInstanceIds, result);
            }
            finally
            {
                _peerObserver.EndEdit();
            }
        }
    }

    private IReadOnlyList<InstanceId> ResolveTargets(
        InstanceId sourceInstanceId,
        CommandScope scope)
    {
        return scope switch
        {
            CommandScope.FocusedBank => ResolveFocusedTarget(sourceInstanceId),
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

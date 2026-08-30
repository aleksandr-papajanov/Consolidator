using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
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
    private readonly ContextualBankResolver _bankResolver;

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
        _bankResolver = new ContextualBankResolver(topologyIndex);
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

            var requestedTargetInstanceIds = command is ITargetedInstanceCommand
                    { TargetInstanceId: { } explicitTarget }
                    ? [explicitTarget]
                    : command.Scope is CommandScope.Coordinator
                    ? [sourceInstanceId]
                    : ResolveTargets(sourceInstanceId, command.Scope);
            var bankTarget = command is ITargetedInstanceCommand
                { TargetInstanceId: { } targetInstanceId }
                    ? _bankResolver.Resolve(sourceInstanceId, targetInstanceId)
                    : command is ResetStateCommand or WriteStateCommand
                        ? ResolveSelectedBankTarget(sourceInstanceId)
                        : null;
            var targetInstanceIds = command is ResetStateCommand resetCommand &&
                resetCommand.ResetMode is ResetScope.GroupInstance &&
                bankTarget?.Group?.Members is { } members
                    ? members
                        .Select(member => member.InstanceId)
                        .Distinct()
                        .ToArray()
                    : requestedTargetInstanceIds;
            _peerObserver.BeginEdit(
                command is ITargetedInstanceCommand
                    ? bankTarget?.TargetBank
                    : _topologyIndex.ResolveFocusedBankAddress(sourceInstanceId),
                UsesGroupPropagation(command));
            try
            {
                var result = await _executor.ExecuteAsync(
                    targetInstanceIds,
                    command,
                    bankTarget,
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

    private static bool IsScopedGroupReset(ResetStateCommand command) =>
        command.ResetMode is ResetScope.Group;

    private static bool UsesGroupPropagation<TResult>(IInstanceCommand<TResult> command) =>
        command switch
        {
            ResetStateCommand reset => IsScopedGroupReset(reset),
            WriteStateCommand write => write.WriteMode is WriteScope.Group,
            _ => true
        };

    private ContextualBankTarget? ResolveSelectedBankTarget(InstanceId sourceInstanceId)
    {
        var selectedBank = _instanceRegistry.FindInstance(sourceInstanceId)?
            .State.Transient.Selection.SelectedBank;
        return selectedBank is { } bank
            ? _bankResolver.Resolve(sourceInstanceId, bank.InstanceId)
            : null;
    }

    private IReadOnlyList<InstanceId> ResolveFocusedTarget(InstanceId sourceInstanceId)
    {
        var instanceId = _topologyIndex.ResolveFocusedInstanceId(sourceInstanceId);
        return instanceId is { } value
            ? [value]
            : Array.Empty<InstanceId>();
    }
}

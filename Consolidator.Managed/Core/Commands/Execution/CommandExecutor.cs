using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.Services.PerInstance;
using Consolidator.Managed.Core.Topology;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Commands.Execution;

public sealed class CommandExecutor
{
    private readonly InstanceRegistry _instanceRegistry;
    private readonly ICommandDispatcher _commandDispatcher;
    private readonly DspStateChangeTracker _dspChanges;

    internal CommandExecutor(
        InstanceRegistry instanceRegistry,
        ICommandDispatcher commandDispatcher,
        DspStateChangeTracker dspChanges)
    {
        _instanceRegistry = instanceRegistry;
        _commandDispatcher = commandDispatcher;
        _dspChanges = dspChanges;
    }

    public async ValueTask<CommandExecutionResult<TResult>> ExecuteAsync<TResult>(
        IReadOnlyList<InstanceId> targetInstanceIds,
        IInstanceCommand<TResult> command,
        ContextualBankTarget? bankTarget,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(targetInstanceIds);
        ArgumentNullException.ThrowIfNull(command);
        cancellationToken.ThrowIfCancellationRequested();

        if (targetInstanceIds.Count == 0)
        {
            return CommandExecutionResult<TResult>.Failure(
                0,
                0,
                "No command target was resolved.");
        }

        var instances = targetInstanceIds
            .Select(target => _instanceRegistry.FindInstance(target))
            .ToArray();
        if (instances.Any(instance => instance is null))
        {
            return CommandExecutionResult<TResult>.Failure(
                targetInstanceIds.Count,
                0,
                "One or more command targets were stopped.");
        }

        var result = await ExecuteOnTargets(
            instances.Select(instance => instance!).ToArray(),
            command,
            bankTarget,
            cancellationToken);
        var stateChanged = command is ResetStateCommand ||
            (command is WriteStateCommand ||
                command is SetInstanceMuteCommand ||
                command is SetInstanceSoloCommand ||
                command is SetInstanceBypassCommand ||
                command is SetProcessorBypassCommand) &&
            result.Value is StateWriteStatus.Applied;
        if (result.Succeeded && stateChanged)
        {
            var affectedInstanceIds = _dspChanges.Drain();
            _instanceRegistry.PublishDspStates(affectedInstanceIds);
        }

        return result;
    }

    private async ValueTask<CommandExecutionResult<TResult>> ExecuteOnTargets<TResult>(
        IReadOnlyList<ManagedInstance> instances,
        IInstanceCommand<TResult> command,
        ContextualBankTarget? bankTarget,
        CancellationToken cancellationToken)
    {
        var results = await Task.WhenAll(
            instances.Select(instance => ExecuteOnInstance<TResult>(
                instance,
                command,
                bankTarget,
                cancellationToken)
                .AsTask()));

        return AggregateResults(results);
    }

    private async ValueTask<InstanceCommandExecution<TResult>> ExecuteOnInstance<TResult>(
        ManagedInstance instance,
        IInstanceCommand<TResult> command,
        ContextualBankTarget? bankTarget,
        CancellationToken cancellationToken)
    {
        try
        {
            var value = await instance.ExecuteAsync(
                    state => _commandDispatcher.DispatchAsync(
                    command,
                    new InstanceCommandContext(
                        instance.InstanceId,
                        state,
                        bankTarget),
                    cancellationToken),
                cancellationToken);
            return new InstanceCommandExecution<TResult>(value, null);
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception exception)
        {
            return new InstanceCommandExecution<TResult>(
                default,
                exception.Message);
        }
    }

    private static CommandExecutionResult<TResult> AggregateResults<TResult>(
        IReadOnlyList<InstanceCommandExecution<TResult>> results)
    {
        var failed = results.FirstOrDefault(result => !result.Succeeded);
        if (failed is not null)
        {
            return CommandExecutionResult<TResult>.Failure(
                results.Count,
                results.Count(result => result.Succeeded),
                failed.Error ?? "Command execution failed.");
        }

        return CommandExecutionResult<TResult>.Success(
            results[0].Value,
            results.Count,
            results.Count);
    }

    private sealed record InstanceCommandExecution<TResult>(
        TResult? Value,
        string? Error)
    {
        public bool Succeeded => Error is null;
    }
}




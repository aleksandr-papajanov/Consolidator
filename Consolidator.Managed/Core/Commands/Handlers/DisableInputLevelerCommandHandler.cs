using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class DisableInputLevelerCommandHandler
    : CommandHandler<DisableInputLevelerCommand, StateWriteStatus>
{
    private readonly InstanceRegistry _instances;
    private readonly InstanceControlTargetResolver _targets;
    private readonly IInputLevelerTelemetry _telemetry;
    private readonly StateHistory _history;

    public DisableInputLevelerCommandHandler(
        InstanceRegistry instances,
        InstanceControlTargetResolver targets,
        IInputLevelerTelemetry telemetry,
        StateHistory history)
    {
        _instances = instances;
        _targets = targets;
        _telemetry = telemetry;
        _history = history;
    }

    public override ValueTask<StateWriteStatus> HandleAsync(
        DisableInputLevelerCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (command.TargetInstanceId is not { } targetInstanceId ||
            !_targets.TryResolve(
                command.TargetScope,
                context,
                targetInstanceId,
                out var ids))
        {
            return ValueTask.FromResult(StateWriteStatus.Rejected);
        }

        var instances = ids
            .Select(_instances.FindInstance)
            .Where(instance => instance is not null)
            .Select(instance => instance!)
            .ToArray();
        if (instances.Length != ids.Count)
        {
            return ValueTask.FromResult(StateWriteStatus.Rejected);
        }

        var targets = instances.Select(instance =>
        {
            var level = instance.State.Dsp.InputGain.Level.Value;
            var hasLevel = _telemetry.TryReadSlowLevel(
                instance.InstanceId,
                out level);
            return new
            {
                Instance = instance,
                HasLevel = hasLevel,
                Level = level
            };
        }).ToArray();
        var changed = targets.Any(target =>
            target.Instance.State.Dsp.InputGain.Leveler.Value ||
            target.HasLevel &&
            target.Instance.State.Dsp.InputGain.Level.Value != target.Level);
        if (!changed)
        {
            return ValueTask.FromResult(StateWriteStatus.Unchanged);
        }

        using var transaction = _history.BeginTransaction();
        foreach (var target in targets)
        {
            if (target.HasLevel)
            {
                target.Instance.State.Dsp.InputGain.Level.Prepare(
                    target.Level,
                    transaction);
            }
            target.Instance.State.Dsp.InputGain.Leveler.Prepare(
                false,
                transaction);
        }
        transaction.Commit();
        return ValueTask.FromResult(StateWriteStatus.Applied);
    }
}

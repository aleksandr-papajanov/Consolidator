using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Notifications;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class InitializeUiCommandHandler
    : CommandHandler<InitializeUiCommand, UiInitializationResult>
{
    private readonly StateHistory _history;
    private readonly HistoryStatePublisher _historyStatePublisher;

    public InitializeUiCommandHandler(
        StateHistory history,
        HistoryStatePublisher historyStatePublisher)
    {
        _history = history;
        _historyStatePublisher = historyStatePublisher;
    }

    public override ValueTask<UiInitializationResult> HandleAsync(
        InitializeUiCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        _historyStatePublisher.Publish(_history.Snapshot);
        return ValueTask.FromResult(
            new UiInitializationResult(
                context.InstanceId.Value,
                context.State.Transient.SnapshotContext));
    }
}

internal sealed class ObserveTargetCommandHandler
    : CommandHandler<ObserveTargetCommand, TargetStateSnapshotResult>
{
    private readonly InstanceRegistry _registry;
    private readonly TargetStateProjector _projector;

    public ObserveTargetCommandHandler(
        InstanceRegistry registry,
        TargetStateProjector projector)
    {
        _registry = registry;
        _projector = projector;
    }

    public override ValueTask<TargetStateSnapshotResult> HandleAsync(
        ObserveTargetCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var target = _registry.FindInstance(command.TargetInstanceId)
            ?? throw new KeyNotFoundException(
                $"Observed instance was not found: {command.TargetInstanceId}.");
        if ((int)command.BankId < 0 ||
            (int)command.BankId >= target.State.Instance.Banks.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(command.BankId));
        }

        context.State.Transient.FocusedBank = new BankAddress(
            command.TargetInstanceId,
            (int)command.BankId);
        context.State.Transient.SnapshotContext = command.SnapshotContext;
        if (command.SnapshotContext == ProcessorId.Equalizer)
        {
            _registry.PublishAnalyzerState(
                command.TargetInstanceId,
                command.SnapshotContext);
        }
        return ValueTask.FromResult(
            _projector.Project(target.State, command.BankId, command.SnapshotContext));
    }
}

internal sealed class SetInstanceActiveCommandHandler
    : CommandHandler<SetInstanceActiveCommand, CommandAcknowledgement>
{
    private readonly InstanceActivityCoordinator _activity;

    public SetInstanceActiveCommandHandler(InstanceActivityCoordinator activity)
    {
        _activity = activity;
    }

    public override ValueTask<CommandAcknowledgement> HandleAsync(
        SetInstanceActiveCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        _activity.SetInstanceActive(context.InstanceId, command.Active);
        return ValueTask.FromResult(new CommandAcknowledgement());
    }
}

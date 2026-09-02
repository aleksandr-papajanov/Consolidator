using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Services;
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
                context.State.Transient.Selection.SelectedProcessor));
    }
}

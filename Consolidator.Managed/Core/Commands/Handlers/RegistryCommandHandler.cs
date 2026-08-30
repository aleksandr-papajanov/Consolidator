using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Protocol.Notifications;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class ReadRegistryCommandHandler
    : CommandHandler<ReadRegistryCommand, RegistrySnapshotResult>
{
    private readonly InstanceRegistry _registry;
    private readonly RegistryChangePublisher _registryChanges;
    private readonly ProcessorMarkerPublisher _processorMarkers;

    public ReadRegistryCommandHandler(
        InstanceRegistry registry,
        RegistryChangePublisher registryChanges,
        ProcessorMarkerPublisher processorMarkers)
    {
        _registry = registry;
        _registryChanges = registryChanges;
        _processorMarkers = processorMarkers;
    }

    public override ValueTask<RegistrySnapshotResult> HandleAsync(
        ReadRegistryCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var snapshot = _processorMarkers.Initialize(
            context.InstanceId,
            _registry.CreateSnapshot());
        _registryChanges.RegisterObserver(context.InstanceId.Value);
        return ValueTask.FromResult(snapshot);
    }
}

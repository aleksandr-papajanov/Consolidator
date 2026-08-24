using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Commands.Results;
using Consolidator.Managed.Core.Services.Instances;

namespace Consolidator.Managed.Core.Commands.Handlers;

public sealed class ReadRegistryCommandHandler
    : CommandHandler<ReadRegistryCommand, RegistrySnapshotResult>
{
    private readonly InstanceRegistry _registry;

    public ReadRegistryCommandHandler(InstanceRegistry registry)
    {
        _registry = registry;
    }

    public override ValueTask<RegistrySnapshotResult> HandleAsync(
        ReadRegistryCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return ValueTask.FromResult(_registry.CreateSnapshot());
    }
}

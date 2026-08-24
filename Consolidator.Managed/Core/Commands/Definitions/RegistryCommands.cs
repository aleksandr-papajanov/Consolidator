using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Results;

namespace Consolidator.Managed.Core.Commands.Definitions;

public sealed record ReadRegistryCommand : IInstanceCommand<RegistrySnapshotResult>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

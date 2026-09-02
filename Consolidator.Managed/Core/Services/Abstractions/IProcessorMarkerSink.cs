using Consolidator.Managed.Core.Commands.Results;

namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IProcessorMarkerSink
{
    RegistrySnapshotResult Initialize(
        InstanceId viewerInstanceId,
        RegistrySnapshotResult snapshot);
}
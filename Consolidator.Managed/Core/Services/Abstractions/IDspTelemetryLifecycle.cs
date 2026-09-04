namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IDspTelemetryLifecycle
{
    void SetInstanceActive(InstanceId instanceId, bool active);
}

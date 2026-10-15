namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IGainStageTelemetry
{
    bool TryReadSlowLevel(InstanceId instanceId, bool output, out float levelDb);
}

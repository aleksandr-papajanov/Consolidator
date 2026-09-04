namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IInputLevelerTelemetry
{
    bool TryReadSlowLevel(InstanceId instanceId, out float levelDb);
}

namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IAnalyzerLifecycle
{
    void SetInstanceActive(InstanceId instanceId, bool active);

    void RemoveInstance(InstanceId instanceId);
}
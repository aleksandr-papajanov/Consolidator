namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IInstancePresentationLifecycle
{
    void SetActive(ulong instanceId, bool active);

    void Unregister(ulong instanceId);
}
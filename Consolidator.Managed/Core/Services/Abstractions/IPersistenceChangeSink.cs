namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IPersistenceChangeSink
{
    void Publish(StateValueChanged change);

    IDisposable Suppress(InstanceId instanceId);
}
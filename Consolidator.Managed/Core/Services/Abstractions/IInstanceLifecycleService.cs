using Consolidator.Managed.Core.Dsp;

namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IInstanceLifecycleService
{
    InstanceId RegisterInstance(
        IDspStatePublisher dspPublisher);

    void UnregisterInstance(InstanceId instanceId);
}





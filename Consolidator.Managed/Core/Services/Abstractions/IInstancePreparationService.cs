using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IInstancePreparationService
{
    void Prepare(
        InstanceId instanceId,
        double sampleRate,
        nuint maximumFrameCount);
}





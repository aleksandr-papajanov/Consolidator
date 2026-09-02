namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IInstancePreparationHandler
{
    void Prepare(
        InstanceId instanceId,
        double sampleRate,
        nuint maximumFrameCount);
}





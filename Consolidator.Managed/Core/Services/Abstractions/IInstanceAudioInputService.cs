using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.Services.Abstractions;

public unsafe interface IInstanceAudioInputService
{
    void ReceiveAudio(
        InstanceId instanceId,
        double* mainLeft,
        double* mainRight,
        double* referenceLeft,
        double* referenceRight,
        nuint frameCount);
}





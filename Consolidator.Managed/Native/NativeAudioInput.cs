using Consolidator.Managed.Core.Services.Abstractions;

namespace Consolidator.Managed.Native;

public unsafe sealed class NativeAudioInput
{
    private readonly InstanceId _instanceId;
    private readonly IInstanceAudioInputService _audioInputService;

    public NativeAudioInput(
        InstanceId instanceId,
        IInstanceAudioInputService audioInputService)
    {
        _instanceId = instanceId;
        _audioInputService = audioInputService;
    }

    public void ReceiveAudio(
        double* mainLeft,
        double* mainRight,
        double* referenceLeft,
        double* referenceRight,
        nuint frameCount)
    {
        _audioInputService.ReceiveAudio(
            _instanceId,
            mainLeft,
            mainRight,
            referenceLeft,
            referenceRight,
            frameCount);
    }
}





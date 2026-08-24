namespace Consolidator.Managed.Core.Services;

using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State;

public unsafe sealed class InstanceAudioInputService : IInstanceAudioInputService
{
    private readonly IReadOnlyList<IInstanceAudioInputHandler> _handlers;

    public InstanceAudioInputService(
        IEnumerable<IInstanceAudioInputHandler> handlers)
    {
        _handlers = handlers.ToArray();
    }

    public void ReceiveAudio(
        InstanceId instanceId,
        double* mainLeft,
        double* mainRight,
        double* referenceLeft,
        double* referenceRight,
        nuint frameCount)
    {
        foreach (var handler in _handlers)
        {
            handler.ReceiveAudio(
                instanceId,
                mainLeft,
                mainRight,
                referenceLeft,
                referenceRight,
                frameCount);
        }
    }
}





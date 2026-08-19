using Consolidator.Managed.Core.Instances;

namespace Consolidator.Managed.Native;

public unsafe sealed class NativeAudioInput
{
    private readonly ConsolidatorInstance _instance;

    public NativeAudioInput(ConsolidatorInstance instance)
    {
        ArgumentNullException.ThrowIfNull(instance);
        _instance = instance;
    }

    public void ReceiveAudio(
        double* mainLeft,
        double* mainRight,
        double* referenceLeft,
        double* referenceRight,
        nuint frameCount)
    {
        _instance.ReceiveAudio(
            mainLeft,
            mainRight,
            referenceLeft,
            referenceRight,
            frameCount);
    }
}

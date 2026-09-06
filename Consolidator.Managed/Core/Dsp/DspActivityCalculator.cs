namespace Consolidator.Managed.Core.Dsp;

public static class DspActivityCalculator
{
    public const float ActivityEpsilon = 0.0001F;

    public static bool IsInputGainActive(
        float inputLevel,
        float inputWidth,
        bool inputLeveler) =>
        MathF.Abs(inputLevel) > ActivityEpsilon ||
        MathF.Abs(inputWidth - 1.0F) > ActivityEpsilon ||
        inputLeveler;
}
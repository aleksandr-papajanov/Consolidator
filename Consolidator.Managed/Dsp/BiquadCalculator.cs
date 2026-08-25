namespace Consolidator.Managed.Dsp;

public enum BiquadType
{
    Bell,
    LowShelf,
    HighShelf,
    Gain
}

public readonly record struct BiquadCoefficients(
    double B0,
    double B1,
    double B2,
    double A1,
    double A2);

public static class BiquadCalculator
{
    public static BiquadCoefficients Calculate(
        BiquadType type,
        double frequencyHz,
        double q,
        double gainDb,
        double sampleRate)
    {
        if (type == BiquadType.Gain)
        {
            return new BiquadCoefficients(
                Math.Pow(10.0, gainDb / 20.0),
                0.0,
                0.0,
                0.0,
                0.0);
        }

        var omega = 2.0 * Math.PI * frequencyHz / sampleRate;
        var sine = Math.Sin(omega);
        var cosine = Math.Cos(omega);
        var alpha = sine / (2.0 * q);
        var amplitude = Math.Pow(10.0, gainDb / 40.0);

        if (type == BiquadType.Bell)
        {
            var inverseA0 = 1.0 / (1.0 + alpha / amplitude);
            return new BiquadCoefficients(
                (1.0 + alpha * amplitude) * inverseA0,
                -2.0 * cosine * inverseA0,
                (1.0 - alpha * amplitude) * inverseA0,
                -2.0 * cosine * inverseA0,
                (1.0 - alpha / amplitude) * inverseA0);
        }

        var gain = amplitude;
        var gainRootTerm = 2.0 * Math.Sqrt(gain) * alpha;
        if (type == BiquadType.LowShelf)
        {
            var inverseA0 = 1.0 / ((gain + 1.0) + (gain - 1.0) * cosine + gainRootTerm);
            return new BiquadCoefficients(
                gain * ((gain + 1.0) - (gain - 1.0) * cosine + gainRootTerm) * inverseA0,
                2.0 * gain * ((gain - 1.0) - (gain + 1.0) * cosine) * inverseA0,
                gain * ((gain + 1.0) - (gain - 1.0) * cosine - gainRootTerm) * inverseA0,
                -2.0 * ((gain - 1.0) + (gain + 1.0) * cosine) * inverseA0,
                ((gain + 1.0) + (gain - 1.0) * cosine - gainRootTerm) * inverseA0);
        }

        var highShelfInverseA0 = 1.0 / ((gain + 1.0) - (gain - 1.0) * cosine + gainRootTerm);
        return new BiquadCoefficients(
            gain * ((gain + 1.0) + (gain - 1.0) * cosine + gainRootTerm) * highShelfInverseA0,
            -2.0 * gain * ((gain - 1.0) + (gain + 1.0) * cosine) * highShelfInverseA0,
            gain * ((gain + 1.0) + (gain - 1.0) * cosine - gainRootTerm) * highShelfInverseA0,
            2.0 * ((gain - 1.0) - (gain + 1.0) * cosine) * highShelfInverseA0,
            ((gain + 1.0) - (gain - 1.0) * cosine - gainRootTerm) * highShelfInverseA0);
    }
}

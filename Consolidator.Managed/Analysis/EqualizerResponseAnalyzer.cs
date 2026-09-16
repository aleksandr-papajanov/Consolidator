using System.Numerics;

using Consolidator.Managed.Core.Dsp;
using MathNet.Numerics.IntegralTransforms;

namespace Consolidator.Managed.Analysis;

internal static class EqualizerResponseAnalyzer
{
    private const int CurvePointCount = 512;
    private const int ImpulseFftSize = 32768;
    private const double FrequencyMinimum = 20.0;
    private const double FrequencyMaximum = 20000.0;
    private const double MinimumMagnitude = 1.0e-12;

    public static EqualizerCurve Analyze(
        IReadOnlyList<EqualizerFilterSnapshot> filters,
        double sampleRate)
    {
        ArgumentNullException.ThrowIfNull(filters);
        ValidateSampleRate(sampleRate);

        var impulse = new double[ImpulseFftSize];
        impulse[0] = 1.0;
        foreach (var filter in filters)
        {
            ApplyFilter(impulse, filter, sampleRate);
        }

        var fft = impulse.Select(value => new Complex(value, 0.0)).ToArray();
        Fourier.Forward(fft, FourierOptions.Matlab);
        var frequencies = CreateFrequencies(sampleRate);
        var decibels = frequencies
            .Select(frequency => DecibelsAt(fft, frequency, sampleRate))
            .ToArray();
        return new EqualizerCurve(frequencies, decibels, sampleRate);
    }

    public static EqualizerCurve CalculateAtFrequencies(
        IReadOnlyList<EqualizerFilterSnapshot> filters,
        IReadOnlyList<double> frequencies,
        double sampleRate)
    {
        ArgumentNullException.ThrowIfNull(filters);
        ArgumentNullException.ThrowIfNull(frequencies);
        ValidateSampleRate(sampleRate);

        var decibels = new double[frequencies.Count];
        foreach (var filter in filters)
        {
            if (!filter.Active || Math.Abs(filter.GainDb) <= double.Epsilon)
            {
                continue;
            }

            for (var index = 0; index < frequencies.Count; index++)
            {
                decibels[index] += CalculateDecibelsAt(
                    filter,
                    frequencies[index],
                    sampleRate);
            }
        }

        return new EqualizerCurve(frequencies, decibels, sampleRate);
    }

    internal static IReadOnlyList<double> CreateApproximationFrequencies(
        double sampleRate) => CreateFrequencies(sampleRate);

    private static void ApplyFilter(
        double[] impulse,
        EqualizerFilterSnapshot filter,
        double sampleRate)
    {
        if (!filter.Active || Math.Abs(filter.GainDb) <= double.Epsilon)
        {
            return;
        }

        if (filter.Type is DspFilterType.Gain)
        {
            var amplitude = Math.Pow(10.0, filter.GainDb / 20.0);
            for (var index = 0; index < impulse.Length; index++)
            {
                impulse[index] *= amplitude;
            }
            return;
        }

        if (filter.Type is DspFilterType.Tilt)
        {
            ProcessBiquad(
                impulse,
                CalculateShelf(filter, sampleRate, false, -filter.GainDb));
            ProcessBiquad(
                impulse,
                CalculateShelf(filter, sampleRate, true, filter.GainDb));
            return;
        }

        var coefficients = filter.Type switch
        {
            DspFilterType.LowShelf =>
                CalculateShelf(filter, sampleRate, false, filter.GainDb),
            DspFilterType.HighShelf =>
                CalculateShelf(filter, sampleRate, true, filter.GainDb),
            DspFilterType.Bell => CalculateBell(filter, sampleRate),
            _ => throw new InvalidOperationException("Unknown equalizer filter type.")
        };
        ProcessBiquad(impulse, coefficients);
    }

    private static void ProcessBiquad(
        double[] samples,
        BiquadCoefficients coefficients)
    {
        var input1 = 0.0;
        var input2 = 0.0;
        var output1 = 0.0;
        var output2 = 0.0;
        for (var index = 0; index < samples.Length; index++)
        {
            var input = samples[index];
            var output = coefficients.B0 * input +
                coefficients.B1 * input1 +
                coefficients.B2 * input2 -
                coefficients.A1 * output1 -
                coefficients.A2 * output2;
            samples[index] = output;
            input2 = input1;
            input1 = input;
            output2 = output1;
            output1 = output;
        }
    }

    private static double DecibelsAt(
        IReadOnlyList<Complex> fft,
        double frequency,
        double sampleRate)
    {
        var bin = frequency * ImpulseFftSize / sampleRate;
        var lower = Math.Clamp((int)Math.Floor(bin), 0, fft.Count - 1);
        var upper = Math.Min(lower + 1, fft.Count - 1);
        var fraction = bin - lower;
        var magnitude = fft[lower].Magnitude +
            (fft[upper].Magnitude - fft[lower].Magnitude) * fraction;
        return 20.0 * Math.Log10(Math.Max(magnitude, MinimumMagnitude));
    }

    private static double[] CreateFrequencies(double sampleRate)
    {
        var frequencies = new double[CurvePointCount];
        var maximumFrequency = Math.Min(FrequencyMaximum, sampleRate * 0.499);
        for (var index = 0; index < frequencies.Length; index++)
        {
            var normalized = (double)index / (frequencies.Length - 1);
            frequencies[index] = FrequencyMinimum * Math.Pow(
                maximumFrequency / FrequencyMinimum,
                normalized);
        }

        return frequencies;
    }

    private static double CalculateDecibelsAt(
        EqualizerFilterSnapshot filter,
        double frequency,
        double sampleRate)
    {
        if (filter.Type is DspFilterType.Gain)
        {
            return filter.GainDb;
        }

        if (filter.Type is DspFilterType.Tilt)
        {
            return MagnitudeDecibels(
                    CalculateShelf(filter, sampleRate, false, -filter.GainDb),
                    frequency,
                    sampleRate) +
                MagnitudeDecibels(
                    CalculateShelf(filter, sampleRate, true, filter.GainDb),
                    frequency,
                    sampleRate);
        }

        var coefficients = filter.Type switch
        {
            DspFilterType.LowShelf =>
                CalculateShelf(filter, sampleRate, false, filter.GainDb),
            DspFilterType.HighShelf =>
                CalculateShelf(filter, sampleRate, true, filter.GainDb),
            DspFilterType.Bell => CalculateBell(filter, sampleRate),
            _ => throw new InvalidOperationException("Unknown equalizer filter type.")
        };
        return MagnitudeDecibels(coefficients, frequency, sampleRate);
    }

    private static BiquadCoefficients CalculateShelf(
        EqualizerFilterSnapshot filter,
        double sampleRate,
        bool highShelf,
        double gainDb)
    {
        var frequency = Math.Min(filter.FrequencyHz, sampleRate * 0.499);
        var omega = 2.0 * Math.PI * frequency / sampleRate;
        var sine = Math.Sin(omega);
        var cosine = Math.Cos(omega);
        var amplitude = Math.Pow(10.0, gainDb / 40.0);
        var alpha = sine / (2.0 * filter.Q);
        var beta = 2.0 * Math.Sqrt(amplitude) * alpha;
        var b0 = highShelf
            ? amplitude * ((amplitude + 1.0) + (amplitude - 1.0) * cosine + beta)
            : amplitude * ((amplitude + 1.0) - (amplitude - 1.0) * cosine + beta);
        var b1 = highShelf
            ? -2.0 * amplitude * ((amplitude - 1.0) + (amplitude + 1.0) * cosine)
            : 2.0 * amplitude * ((amplitude - 1.0) - (amplitude + 1.0) * cosine);
        var b2 = highShelf
            ? amplitude * ((amplitude + 1.0) + (amplitude - 1.0) * cosine - beta)
            : amplitude * ((amplitude + 1.0) - (amplitude - 1.0) * cosine - beta);
        var a0 = highShelf
            ? (amplitude + 1.0) - (amplitude - 1.0) * cosine + beta
            : (amplitude + 1.0) + (amplitude - 1.0) * cosine + beta;
        var a1 = highShelf
            ? 2.0 * ((amplitude - 1.0) - (amplitude + 1.0) * cosine)
            : -2.0 * ((amplitude - 1.0) + (amplitude + 1.0) * cosine);
        var a2 = highShelf
            ? (amplitude + 1.0) - (amplitude - 1.0) * cosine - beta
            : (amplitude + 1.0) + (amplitude - 1.0) * cosine - beta;
        return new BiquadCoefficients(
            b0 / a0,
            b1 / a0,
            b2 / a0,
            a1 / a0,
            a2 / a0);
    }

    private static BiquadCoefficients CalculateBell(
        EqualizerFilterSnapshot filter,
        double sampleRate)
    {
        var frequency = Math.Min(filter.FrequencyHz, sampleRate * 0.499);
        var omega = 2.0 * Math.PI * frequency / sampleRate;
        var sine = Math.Sin(omega);
        var cosine = Math.Cos(omega);
        var alpha = sine / (2.0 * filter.Q);
        var amplitude = Math.Pow(10.0, filter.GainDb / 40.0);
        var inverseA0 = 1.0 / (1.0 + alpha / amplitude);
        return new BiquadCoefficients(
            (1.0 + alpha * amplitude) * inverseA0,
            -2.0 * cosine * inverseA0,
            (1.0 - alpha * amplitude) * inverseA0,
            -2.0 * cosine * inverseA0,
            (1.0 - alpha / amplitude) * inverseA0);
    }

    private static double MagnitudeDecibels(
        BiquadCoefficients coefficients,
        double frequency,
        double sampleRate)
    {
        var omega = 2.0 * Math.PI * frequency / sampleRate;
        var cosine = Math.Cos(omega);
        var sine = Math.Sin(omega);
        var cosine2 = Math.Cos(2.0 * omega);
        var sine2 = Math.Sin(2.0 * omega);
        var numeratorReal = coefficients.B0 + coefficients.B1 * cosine +
            coefficients.B2 * cosine2;
        var numeratorImaginary = -coefficients.B1 * sine -
            coefficients.B2 * sine2;
        var denominatorReal = 1.0 + coefficients.A1 * cosine +
            coefficients.A2 * cosine2;
        var denominatorImaginary = -coefficients.A1 * sine -
            coefficients.A2 * sine2;
        var numeratorMagnitude = Math.Sqrt(
            numeratorReal * numeratorReal + numeratorImaginary * numeratorImaginary);
        var denominatorMagnitude = Math.Sqrt(
            denominatorReal * denominatorReal + denominatorImaginary * denominatorImaginary);
        var magnitude = numeratorMagnitude /
            Math.Max(denominatorMagnitude, MinimumMagnitude);
        return 20.0 * Math.Log10(Math.Max(magnitude, MinimumMagnitude));
    }

    private static void ValidateSampleRate(double sampleRate)
    {
        if (!double.IsFinite(sampleRate) || sampleRate <= 0)
        {
            throw new ArgumentOutOfRangeException(nameof(sampleRate));
        }
    }

    private readonly record struct BiquadCoefficients(
        double B0,
        double B1,
        double B2,
        double A1,
        double A2);
}

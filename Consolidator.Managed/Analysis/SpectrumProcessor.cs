using System.Numerics;

using MathNet.Numerics.IntegralTransforms;

namespace Consolidator.Managed.Analysis;

internal static class SpectrumProcessor
{
    private const double MinimumDecibels = -90.0;

    public static double[] CreateWindow(int size)
    {
        var window = new double[size];
        for (var index = 0; index < window.Length; index++)
        {
            window[index] = 0.5 - 0.5 * Math.Cos(2.0 * Math.PI * index / (window.Length - 1));
        }

        return window;
    }

    public static void Process(AudioCapture capture, int fftSize)
    {
        Fourier.Forward(capture.MainFft, FourierOptions.Matlab);
        Fourier.Forward(capture.ReferenceFft, FourierOptions.Matlab);
        FillSpectrum(capture.MainFft, capture.MainSpectrum, fftSize);
        FillSpectrum(capture.ReferenceFft, capture.ReferenceSpectrum, fftSize);
    }

    private static void FillSpectrum(
        Complex[] fft,
        float[] spectrum,
        int fftSize)
    {
        for (var index = 0; index < spectrum.Length; index++)
        {
            var magnitude = fft[index].Magnitude / fftSize;
            var decibels = 20.0 * Math.Log10(Math.Max(magnitude, 1e-12));
            spectrum[index] = (float)Math.Clamp(
                1.0 - ((decibels - MinimumDecibels) / -MinimumDecibels),
                0.0,
                1.0);
        }
    }
}
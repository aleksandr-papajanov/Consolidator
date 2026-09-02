using System.Numerics;

namespace Consolidator.Managed.Analysis;

internal sealed class AudioCapture
{
    private const int FftSize = 1024;
    private const int ChannelCount = 4;
    private readonly double[][] _channels;
    private readonly int _capacity;
    private int _writeIndex;
    private int _readIndex;

    public Complex[] MainFft { get; } = new Complex[FftSize];
    public Complex[] ReferenceFft { get; } = new Complex[FftSize];
    public float[] MainSpectrum { get; } = new float[FftSize / 2 + 1];
    public float[] ReferenceSpectrum { get; } = new float[FftSize / 2 + 1];

    public AudioCapture(int blockSize, int queueLength)
    {
        _capacity = Math.Max(2048, blockSize * Math.Max(2, queueLength));
        _channels = Enumerable.Range(0, ChannelCount)
            .Select(_ => new double[_capacity])
            .ToArray();
    }

    public unsafe int Enqueue(
        double* mainLeft,
        double* mainRight,
        double* referenceLeft,
        double* referenceRight,
        nuint frameCount)
    {
        var count = (int)Math.Min((nuint)int.MaxValue, frameCount);
        var writeIndex = _writeIndex;
        var readIndex = Volatile.Read(ref _readIndex);
        var available = _capacity - (writeIndex - readIndex);
        var writable = Math.Min(count, available);
        for (var index = 0; index < writable; index++)
        {
            var ringIndex = (writeIndex + index) % _capacity;
            _channels[0][ringIndex] = mainLeft[index];
            _channels[1][ringIndex] = mainRight[index];
            _channels[2][ringIndex] = referenceLeft is null ? 0 : referenceLeft[index];
            _channels[3][ringIndex] = referenceRight is null ? 0 : referenceRight[index];
        }

        if (writable > 0)
        {
            Volatile.Write(ref _writeIndex, writeIndex + writable);
        }

        return count - writable;
    }

    public bool TryReadWindow(
        IReadOnlyList<double> window,
        int windowSize,
        int hopSize)
    {
        var readIndex = _readIndex;
        var writeIndex = Volatile.Read(ref _writeIndex);
        if (writeIndex - readIndex < windowSize)
        {
            return false;
        }

        if (writeIndex - readIndex > windowSize)
        {
            readIndex = writeIndex - windowSize;
        }

        for (var index = 0; index < windowSize; index++)
        {
            var ringIndex = (readIndex + index) % _capacity;
            var multiplier = window[index];
            MainFft[index] = new Complex(
                (_channels[0][ringIndex] + _channels[1][ringIndex]) * 0.5 * multiplier,
                0);
            ReferenceFft[index] = new Complex(
                (_channels[2][ringIndex] + _channels[3][ringIndex]) * 0.5 * multiplier,
                0);
        }

        Volatile.Write(ref _readIndex, readIndex + hopSize);
        return true;
    }
}
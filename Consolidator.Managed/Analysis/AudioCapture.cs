using System.Numerics;

namespace Consolidator.Managed.Analysis;

internal sealed class AudioCapture
{
    private const int FftSize = 1024;
    private const int ChannelCount = 4;
    private readonly double[][] _channels;
    private readonly bool[] _referenceConnections;
    private readonly int _capacity;
    private int _writeIndex;
    private int _readIndex;

    public Complex[] Fft { get; } = new Complex[FftSize];
    public float[] Spectrum { get; } = new float[FftSize / 2 + 1];
    public Complex[] ReferenceFft { get; } = new Complex[FftSize];
    public float[] ReferenceSpectrum { get; } = new float[FftSize / 2 + 1];
    public bool ReferenceConnected { get; private set; }

    public AudioCapture(
        int blockSize,
        int queueLength,
        double sampleRate,
        TimeSpan publicationInterval)
    {
        var backlogLength = Math.Max(2, queueLength);
        var blockCapacity = blockSize * backlogLength;
        var intervalCapacity = (int)Math.Ceiling(
            sampleRate * publicationInterval.TotalSeconds * backlogLength);
        _capacity = Math.Max(2048, Math.Max(blockCapacity, intervalCapacity));
        _channels = Enumerable.Range(0, ChannelCount)
            .Select(_ => new double[_capacity])
            .ToArray();
        _referenceConnections = new bool[_capacity];
    }

    public unsafe int Enqueue(
        double* mainLeft,
        double* mainRight,
        double* referenceLeft,
        double* referenceRight,
        nuint frameCount)
    {
        var count = (int)Math.Min((nuint)int.MaxValue, frameCount);
        var referenceConnected = referenceLeft is not null && referenceRight is not null;
        var writeIndex = _writeIndex;
        var readIndex = Volatile.Read(ref _readIndex);
        var available = _capacity - (writeIndex - readIndex);
        var writable = Math.Min(count, available);
        for (var index = 0; index < writable; index++)
        {
            var ringIndex = (writeIndex + index) % _capacity;
            _channels[0][ringIndex] = mainLeft[index];
            _channels[1][ringIndex] = mainRight[index];
            _channels[2][ringIndex] = referenceConnected ? referenceLeft[index] : 0;
            _channels[3][ringIndex] = referenceConnected ? referenceRight[index] : 0;
            _referenceConnections[ringIndex] = referenceConnected;
        }

        if (writable > 0)
        {
            Volatile.Write(ref _writeIndex, writeIndex + writable);
        }

        return count - writable;
    }

    public bool TryReadWindow(
        IReadOnlyList<double> window,
        int windowSize)
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
            Fft[index] = new Complex(
                (_channels[0][ringIndex] + _channels[1][ringIndex]) * 0.5 * multiplier,
                0);
            ReferenceFft[index] = new Complex(
                (_channels[2][ringIndex] + _channels[3][ringIndex]) * 0.5 * multiplier,
                0);
        }

        ReferenceConnected = _referenceConnections[(writeIndex - 1) % _capacity];
        // This analyzer is latest-window only. Discard samples accumulated while
        // the worker was waiting for its next publication interval.
        Volatile.Write(ref _readIndex, writeIndex);
        return true;
    }

    public void FillSilenceWindow(int windowSize)
    {
        Array.Clear(Fft, 0, windowSize);
        Array.Clear(ReferenceFft, 0, windowSize);
        ReferenceConnected = false;
    }
}

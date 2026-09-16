using System;

using Consolidator.Managed.Analysis;
using Xunit;

namespace Consolidator.Managed.Tests.Analysis;

public sealed class AudioCaptureTests
{
    [Fact]
    public unsafe void CaptureCapacityCoversFourPublicationIntervals()
    {
        const int sampleRate = 96000;
        const int blockSize = 128;
        const int queueLength = 4;
        var interval = TimeSpan.FromMilliseconds(33);
        var sampleCount = (int)Math.Ceiling(
            sampleRate * interval.TotalSeconds * queueLength);
        var mainLeft = new double[sampleCount];
        var mainRight = new double[sampleCount];

        var capture = new AudioCapture(
            blockSize,
            queueLength,
            sampleRate,
            interval);

        fixed (double* left = mainLeft)
        fixed (double* right = mainRight)
        {
            var dropped = capture.Enqueue(
                left,
                right,
                (nuint)sampleCount);

            Assert.Equal(0, dropped);
        }
    }
}

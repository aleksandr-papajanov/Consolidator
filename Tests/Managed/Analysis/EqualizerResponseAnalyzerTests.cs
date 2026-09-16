using Consolidator.Managed.Analysis;
using Consolidator.Managed.Core.Dsp;
using Xunit;

namespace Consolidator.Managed.Tests.Analysis;

public sealed class EqualizerResponseAnalyzerTests
{
    [Fact]
    public void AnalyzerTransformsGeneratedEqualizerImpulseWithMathNetFft()
    {
        var filters = new[]
        {
            new EqualizerFilterSnapshot(
                DspFilterType.Gain,
                0.0,
                6.0,
                0.0,
                true)
        };

        var curve = EqualizerResponseAnalyzer.Analyze(filters, 48000.0);

        Assert.Equal(512, curve.Frequencies.Count);
        Assert.All(curve.Decibels, value => Assert.Equal(6.0, value, 9));
    }
}

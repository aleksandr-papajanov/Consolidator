using System.Linq;
using Consolidator.Managed.Core.Settings;
using Xunit;

namespace Consolidator.Managed.Tests.Contracts;

public sealed class FilterCatalogContractTests
{
    [Fact]
    public void EqualizerUsesTheLegacyFilterLayoutAndDefaults()
    {
        Assert.Equal(
            [
                FilterKind.Gain,
                FilterKind.Tilt,
                FilterKind.LowShelf,
                FilterKind.HighShelf,
                FilterKind.Bell,
                FilterKind.Bell,
                FilterKind.Bell
            ],
            FilterCatalog.Equalizer.Select(definition => definition.Kind));

        Assert.Equal(
            [null, 1000.0F, 100.0F, 10000.0F, 1000.0F, 2000.0F, 4000.0F],
            FilterCatalog.Equalizer.Select(definition => definition.Frequency?.DefaultValue));
        Assert.All(FilterCatalog.Equalizer.Skip(1).Take(3), definition => Assert.Null(definition.Q));
        Assert.All(FilterCatalog.Equalizer.Skip(4), definition => Assert.Equal(0.707F, definition.Q?.DefaultValue));
        Assert.All(FilterCatalog.Equalizer, definition =>
        {
            Assert.Equal(-24.0F, definition.Gain.Range.Minimum);
            Assert.Equal(24.0F, definition.Gain.Range.Maximum);
        });
        Assert.Equal(0.1F, FilterCatalog.Equalizer[4].Q?.Range.Minimum);
        Assert.Equal(10.0F, FilterCatalog.Equalizer[4].Q?.Range.Maximum);
        Assert.Equal(0.707F, FilterCatalog.Equalizer[1].FixedQ);
    }

    [Fact]
    public void DetectorUsesLowShelfAndBellDefaults()
    {
        Assert.Equal(
            [FilterKind.LowShelf, FilterKind.Bell],
            FilterCatalog.Detector.Select(definition => definition.Kind));
        Assert.Equal(
            [100.0F, 1000.0F],
            FilterCatalog.Detector.Select(definition => definition.Frequency?.DefaultValue));
    }

    [Fact]
    public void GainFilterHasNoFrequencyOrQParameter()
    {
        var definition = FilterCatalog.Equalizer[0];

        Assert.Null(definition.Frequency);
        Assert.Null(definition.Q);
        Assert.Equal("gain", FilterCatalog.ToProtocolName(definition.Kind));
    }
}

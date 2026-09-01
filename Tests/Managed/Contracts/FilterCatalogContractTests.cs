using System.Linq;
using Consolidator.Managed.Core.Settings;
using Xunit;

namespace Consolidator.Managed.Tests.Contracts;

public sealed class StateValueDefinitionsContractTests
{
    [Fact]
    public void EqualizerUsesTheLegacyFilterLayoutAndDefaults()
    {
        Assert.IsType<GainFilterDefinition>(StateValueDefinitions.EqualizerDefinitions[0]);
        Assert.IsType<TiltFilterDefinition>(StateValueDefinitions.EqualizerDefinitions[1]);
        Assert.IsType<LowShelfFilterDefinition>(StateValueDefinitions.EqualizerDefinitions[2]);
        Assert.IsType<HighShelfFilterDefinition>(StateValueDefinitions.EqualizerDefinitions[3]);
        Assert.All(
            StateValueDefinitions.EqualizerDefinitions.Skip(4),
            definition => Assert.IsType<BellFilterDefinition>(definition));

        Assert.Equal(1000.0F, ((FrequencyFilterDefinition)StateValueDefinitions.EqualizerDefinitions[1]).Frequency.DefaultValue);
        Assert.Equal(100.0F, ((FrequencyFilterDefinition)StateValueDefinitions.EqualizerDefinitions[2]).Frequency.DefaultValue);
        Assert.Equal(10000.0F, ((FrequencyFilterDefinition)StateValueDefinitions.EqualizerDefinitions[3]).Frequency.DefaultValue);
        Assert.Equal(0.707F, ((BellFilterDefinition)StateValueDefinitions.EqualizerDefinitions[4]).Q.DefaultValue);
        Assert.All(StateValueDefinitions.EqualizerDefinitions, definition =>
        {
            Assert.Equal(-24.0F, definition.Gain.Range.Minimum);
            Assert.Equal(24.0F, definition.Gain.Range.Maximum);
        });
        Assert.Equal(0.1F, ((BellFilterDefinition)StateValueDefinitions.EqualizerDefinitions[4]).Q.Range.Minimum);
        Assert.Equal(10.0F, ((BellFilterDefinition)StateValueDefinitions.EqualizerDefinitions[4]).Q.Range.Maximum);
        Assert.Equal(0.707F, ((FixedQFilterDefinition)StateValueDefinitions.EqualizerDefinitions[1]).FixedQ);
    }

    [Fact]
    public void DetectorUsesLowShelfAndBellDefaults()
    {
        Assert.IsType<LowShelfFilterDefinition>(StateValueDefinitions.DetectorDefinitions[0]);
        Assert.IsType<BellFilterDefinition>(StateValueDefinitions.DetectorDefinitions[1]);
        Assert.Equal(100.0F, ((FrequencyFilterDefinition)StateValueDefinitions.DetectorDefinitions[0]).Frequency.DefaultValue);
        Assert.Equal(1000.0F, ((BellFilterDefinition)StateValueDefinitions.DetectorDefinitions[1]).Frequency.DefaultValue);
    }

    [Fact]
    public void GainFilterHasNoFrequencyOrQParameter()
    {
        var definition = StateValueDefinitions.EqualizerDefinitions[0];

        Assert.IsType<GainFilterDefinition>(definition);
    }
}

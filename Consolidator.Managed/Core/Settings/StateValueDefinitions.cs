using Consolidator.Managed.Core.State;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.Settings;

public static class StateValueDefinitions
{
    static StateValueDefinitions()
    {
        Common = new CommonDefinitions(
            new StateValueDefinition<bool>(false),
            new StateValueDefinition<bool>(
                false,
                RegisterInHistory: false),
            new StateValueDefinition<string>(
                string.Empty,
                RegisterInHistory: false));
        Input = new InputDefinitions(
            new(0.0F, new(-36.0F, 36.0F)),
            new(-18.0F, new(-36.0F, 0.0F)),
            new(100.0F, new(0.0F, 100.0F)));
        Output = new OutputDefinitions(
            new(0.0F, new(-36.0F, 36.0F)),
            new(-1.0F, new(-36.0F, 0.0F)));
        Compressor = new CompressorDefinitions(
            new StateValueDefinition<float>(
                0.0F,
                new FloatRange(0.0F, 1.0F)),
            new StateValueDefinition<float>(
                0.0F,
                new FloatRange(0.0F, 1.0F)),
            new StateValueDefinition<float>(
                0.0F,
                new FloatRange(0.0F, 1.0F)),
            new StateValueDefinition<int>(0),
            new StateValueDefinition<bool>(false),
            new(0.0F, new(-36.0F, 36.0F)));
        Saturator = new SaturatorDefinitions(
            new StateValueDefinition<float>(
                0.0F,
                new FloatRange(0.0F, 1.0F)),
            new(0.5F, new(0.0F, 1.0F)),
            new StateValueDefinition<bool>(false),
            new(0.0F, new(-36.0F, 36.0F)));
        Polish = new PolishDefinitions(
            new StateValueDefinition<float>(
                0.0F,
                new FloatRange(0.0F, 1.0F)),
            new StateValueDefinition<float>(
                0.0F,
                new FloatRange(0.0F, 1.0F)));
        EqualizerDefinitions = Array.AsReadOnly<FilterDefinition>(
        [
            new GainFilterDefinition(
                new(StateNodeIds.Gain, new(0.0F, new(-24.0F, 24.0F)))),
            new TiltFilterDefinition(
                new(StateNodeIds.Frequency, new(1000.0F, new(20.0F, 20000.0F))),
                new(StateNodeIds.Gain, new(0.0F, new(-24.0F, 24.0F))),
                0.707F),
            new LowShelfFilterDefinition(
                new(StateNodeIds.Frequency, new(100.0F, new(20.0F, 20000.0F))),
                new(StateNodeIds.Gain, new(0.0F, new(-24.0F, 24.0F))),
                0.707F),
            new HighShelfFilterDefinition(
                new(StateNodeIds.Frequency, new(10000.0F, new(20.0F, 20000.0F))),
                new(StateNodeIds.Gain, new(0.0F, new(-24.0F, 24.0F))),
                0.707F),
            new BellFilterDefinition(
                new(StateNodeIds.Frequency, new(1000.0F, new(20.0F, 20000.0F))),
                new(StateNodeIds.Q, new(0.707F, new(0.1F, 10.0F))),
                new(StateNodeIds.Gain, new(0.0F, new(-24.0F, 24.0F)))),
            new BellFilterDefinition(
                new(StateNodeIds.Frequency, new(2000.0F, new(20.0F, 20000.0F))),
                new(StateNodeIds.Q, new(0.707F, new(0.1F, 10.0F))),
                new(StateNodeIds.Gain, new(0.0F, new(-24.0F, 24.0F)))),
            new BellFilterDefinition(
                new(StateNodeIds.Frequency, new(4000.0F, new(20.0F, 20000.0F))),
                new(StateNodeIds.Q, new(0.707F, new(0.1F, 10.0F))),
                new(StateNodeIds.Gain, new(0.0F, new(-24.0F, 24.0F))))
        ]);
        DetectorDefinitions = Array.AsReadOnly<FilterDefinition>(
        [
            new LowShelfFilterDefinition(
                new(StateNodeIds.Frequency, new(100.0F, new(20.0F, 20000.0F))),
                new(StateNodeIds.Gain, new(0.0F, new(-24.0F, 24.0F))),
                0.707F),
            new BellFilterDefinition(
                new(StateNodeIds.Frequency, new(1000.0F, new(20.0F, 20000.0F))),
                new(StateNodeIds.Q, new(0.707F, new(0.1F, 10.0F))),
                new(StateNodeIds.Gain, new(0.0F, new(-24.0F, 24.0F))))
        ]);
    }

    public static CommonDefinitions Common { get; }
    public static InputDefinitions Input { get; }
    public static OutputDefinitions Output { get; }
    public static CompressorDefinitions Compressor { get; }
    public static SaturatorDefinitions Saturator { get; }
    public static PolishDefinitions Polish { get; }
    public static IReadOnlyList<FilterDefinition> EqualizerDefinitions { get; }
    public static IReadOnlyList<FilterDefinition> DetectorDefinitions { get; }

    public sealed record CommonDefinitions(
        StateValueDefinition<bool> CopyValue,
        StateValueDefinition<bool> CopyValueWithoutHistory,
        StateValueDefinition<string> Label);

    public sealed record InputDefinitions(
        StateValueDefinition<float> Level,
        StateValueDefinition<float> Target,
        StateValueDefinition<float> Width);

    public sealed record OutputDefinitions(
        StateValueDefinition<float> Level,
        StateValueDefinition<float> Target);

    public sealed record CompressorDefinitions(
        StateValueDefinition<float> Attack,
        StateValueDefinition<float> Sustain,
        StateValueDefinition<float> Compression,
        StateValueDefinition<int> Character,
        StateValueDefinition<bool> Parallel,
        StateValueDefinition<float> OutputDb);

    public sealed record SaturatorDefinitions(
        StateValueDefinition<float> Drive,
        StateValueDefinition<float> Curve,
        StateValueDefinition<bool> Split,
        StateValueDefinition<float> OutputDb);

    public sealed record PolishDefinitions(
        StateValueDefinition<float> Thick,
        StateValueDefinition<float> Air);
}

public sealed record FilterParameterDefinition(
    NodeId Node,
    StateValueDefinition<float> Definition)
{
    public float DefaultValue => Definition.DefaultValue;

    public FloatRange Range => Definition.PhysicalRange!.Value;
}

public abstract record FilterDefinition(
    FilterParameterDefinition Gain)
{
    public abstract IReadOnlyList<FilterParameterDefinition> Parameters { get; }
}

public sealed record GainFilterDefinition(
    FilterParameterDefinition Gain)
    : FilterDefinition(Gain)
{
    public override IReadOnlyList<FilterParameterDefinition> Parameters => [Gain];
}

public abstract record FrequencyFilterDefinition(
    FilterParameterDefinition Frequency,
    FilterParameterDefinition Gain)
    : FilterDefinition(Gain)
{
    public override IReadOnlyList<FilterParameterDefinition> Parameters => [Frequency, Gain];
}

public abstract record FixedQFilterDefinition(
    FilterParameterDefinition Frequency,
    FilterParameterDefinition Gain,
    float FixedQ)
    : FrequencyFilterDefinition(Frequency, Gain);

public sealed record TiltFilterDefinition(
    FilterParameterDefinition Frequency,
    FilterParameterDefinition Gain,
    float FixedQ)
    : FixedQFilterDefinition(Frequency, Gain, FixedQ);

public sealed record LowShelfFilterDefinition(
    FilterParameterDefinition Frequency,
    FilterParameterDefinition Gain,
    float FixedQ)
    : FixedQFilterDefinition(Frequency, Gain, FixedQ);

public sealed record HighShelfFilterDefinition(
    FilterParameterDefinition Frequency,
    FilterParameterDefinition Gain,
    float FixedQ)
    : FixedQFilterDefinition(Frequency, Gain, FixedQ);

public sealed record BellFilterDefinition(
    FilterParameterDefinition Frequency,
    FilterParameterDefinition Q,
    FilterParameterDefinition Gain)
    : FrequencyFilterDefinition(Frequency, Gain)
{
    public override IReadOnlyList<FilterParameterDefinition> Parameters => [Frequency, Q, Gain];
}

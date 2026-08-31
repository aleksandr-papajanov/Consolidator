using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.Settings;

public enum FilterKind
{
    Bell,
    LowShelf,
    HighShelf,
    Tilt,
    Gain
}

public sealed record FilterParameterDefinition(
    float DefaultValue,
    FloatRange Range);

public sealed record FilterDefinition(
    FilterKind Kind,
    FilterParameterDefinition? Frequency,
    FilterParameterDefinition? Q,
    FilterParameterDefinition Gain,
    float FixedQ);

public static class FilterCatalog
{
    public static readonly FilterDefinition[] Equalizer =
    [
        Create(FilterKind.Gain, null, null, 0.0F, 0.707F),
        Create(FilterKind.Tilt, 1000.0F, null, 0.0F, 0.707F),
        Create(FilterKind.LowShelf, 100.0F, null, 0.0F, 0.707F),
        Create(FilterKind.HighShelf, 10000.0F, null, 0.0F, 0.707F),
        Create(FilterKind.Bell, 1000.0F, 0.707F, 0.0F, 0.707F),
        Create(FilterKind.Bell, 2000.0F, 0.707F, 0.0F, 0.707F),
        Create(FilterKind.Bell, 4000.0F, 0.707F, 0.0F, 0.707F)
    ];

    public static readonly FilterDefinition[] Detector =
    [
        Create(FilterKind.LowShelf, 100.0F, null, 0.0F, 0.707F),
        Create(FilterKind.Bell, 1000.0F, 0.707F, 0.0F, 0.707F)
    ];

    public static string ToProtocolName(FilterKind kind) => kind switch
    {
        FilterKind.Bell => "bell",
        FilterKind.LowShelf => "low_shelf",
        FilterKind.HighShelf => "high_shelf",
        FilterKind.Tilt => "tilt",
        FilterKind.Gain => "gain",
        _ => throw new ArgumentOutOfRangeException(nameof(kind))
    };

    public static IReadOnlyList<FilterDefinition> For(ProcessorId processorId) =>
        processorId switch
        {
            ProcessorId.Equalizer => Equalizer,
            ProcessorId.Saturator or ProcessorId.Compressor => Detector,
            _ => Array.Empty<FilterDefinition>()
        };

    private static FilterDefinition Create(
        FilterKind kind,
        float? frequency,
        float? q,
        float gain,
        float fixedQ)
    {
        return new FilterDefinition(
            kind,
            CreateParameter(frequency, DspParameterRanges.FrequencyHz),
            CreateParameter(q, DspParameterRanges.Q),
            new FilterParameterDefinition(gain, DspParameterRanges.FilterGainDb),
            fixedQ);
    }

    private static FilterParameterDefinition? CreateParameter(
        float? defaultValue,
        FloatRange range)
    {
        return defaultValue is { } value
            ? new FilterParameterDefinition(value, range)
            : null;
    }
}

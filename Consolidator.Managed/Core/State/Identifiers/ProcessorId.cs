namespace Consolidator.Managed.Core.State.Identifiers;

public enum ProcessorId
{
    GainStaging,
    Saturator,
    Shaper,
    Compression,
    Equalizer,
    Polish,
}

public static class ProcessorIds
{
    public static ProcessorId Parse(string? value) => value switch
    {
        "gain_staging" => ProcessorId.GainStaging,
        "saturator" => ProcessorId.Saturator,
        "shaper" => ProcessorId.Shaper,
        "compression" => ProcessorId.Compression,
        "equalizer" => ProcessorId.Equalizer,
        "polish" => ProcessorId.Polish,
        _ => throw new FormatException("Invalid processor ID.")
    };

    public static string Encode(ProcessorId processorId) => processorId switch
    {
        ProcessorId.GainStaging => "gain_staging",
        ProcessorId.Saturator => "saturator",
        ProcessorId.Shaper => "shaper",
        ProcessorId.Compression => "compression",
        ProcessorId.Equalizer => "equalizer",
        ProcessorId.Polish => "polish",
        _ => throw new ArgumentOutOfRangeException(nameof(processorId))
    };

    public static IReadOnlyList<ProcessorId> All { get; } =
        [ProcessorId.GainStaging, ProcessorId.Saturator, ProcessorId.Shaper, ProcessorId.Compression,
            ProcessorId.Equalizer, ProcessorId.Polish];
}

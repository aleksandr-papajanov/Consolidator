namespace Consolidator.Managed.Core.State.Identifiers;

public enum ProcessorId
{
    GainStaging,
    Saturator,
    Compressor,
    Equalizer,
    Polish,
}

public static class ProcessorIds
{
    public static ProcessorId Parse(string? value) => value switch
    {
        "gain_staging" => ProcessorId.GainStaging,
        "saturator" => ProcessorId.Saturator,
        "compressor" => ProcessorId.Compressor,
        "equalizer" => ProcessorId.Equalizer,
        "polish" => ProcessorId.Polish,
        _ => throw new FormatException("Invalid processor ID.")
    };

    public static string Encode(ProcessorId processorId) => processorId switch
    {
        ProcessorId.GainStaging => "gain_staging",
        ProcessorId.Saturator => "saturator",
        ProcessorId.Compressor => "compressor",
        ProcessorId.Equalizer => "equalizer",
        ProcessorId.Polish => "polish",
        _ => throw new ArgumentOutOfRangeException(nameof(processorId))
    };

    public static IReadOnlyList<ProcessorId> All { get; } =
        [ProcessorId.GainStaging, ProcessorId.Saturator, ProcessorId.Compressor,
            ProcessorId.Equalizer, ProcessorId.Polish];
}

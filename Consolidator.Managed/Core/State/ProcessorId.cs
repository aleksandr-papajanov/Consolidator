namespace Consolidator.Managed.Core.State;

public enum ProcessorId
{
    Input,
    Saturator,
    Compressor,
    Equalizer,
    Polish,
    Output
}

public static class ProcessorIds
{
    public static ProcessorId Parse(string? value) => value switch
    {
        "input" => ProcessorId.Input,
        "saturator" => ProcessorId.Saturator,
        "compressor" => ProcessorId.Compressor,
        "equalizer" => ProcessorId.Equalizer,
        "polish" => ProcessorId.Polish,
        "output" => ProcessorId.Output,
        _ => throw new FormatException("Invalid processor ID.")
    };

    public static string Encode(ProcessorId processorId) => processorId switch
    {
        ProcessorId.Input => "input",
        ProcessorId.Saturator => "saturator",
        ProcessorId.Compressor => "compressor",
        ProcessorId.Equalizer => "equalizer",
        ProcessorId.Polish => "polish",
        ProcessorId.Output => "output",
        _ => throw new ArgumentOutOfRangeException(nameof(processorId))
    };

    public static IReadOnlyList<ProcessorId> All { get; } =
        [ProcessorId.Input, ProcessorId.Saturator, ProcessorId.Compressor,
            ProcessorId.Equalizer, ProcessorId.Polish, ProcessorId.Output];
}

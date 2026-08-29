namespace Consolidator.Managed.Core.State;

public enum UiSnapshotContext
{
    Input,
    Saturator,
    Compressor,
    Equalizer,
    Output
}

public static class UiSnapshotContexts
{
    public static UiSnapshotContext Parse(string? value) => value switch
    {
        "input" => UiSnapshotContext.Input,
        "saturator" => UiSnapshotContext.Saturator,
        "compressor" => UiSnapshotContext.Compressor,
        "equalizer" => UiSnapshotContext.Equalizer,
        "output" => UiSnapshotContext.Output,
        _ => throw new FormatException("Invalid snapshot context.")
    };

    public static string Encode(UiSnapshotContext context) => context switch
    {
        UiSnapshotContext.Input => "input",
        UiSnapshotContext.Saturator => "saturator",
        UiSnapshotContext.Compressor => "compressor",
        UiSnapshotContext.Equalizer => "equalizer",
        UiSnapshotContext.Output => "output",
        _ => throw new ArgumentOutOfRangeException(nameof(context))
    };
}

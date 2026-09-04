namespace Consolidator.Managed.Native;

internal readonly record struct DspTelemetrySnapshot(
    uint Sequence,
    uint ValuesPresent,
    float InputLevelDb,
    float InputLevelFastDb,
    float OutputLevelDb);

namespace Consolidator.Managed.Native;

internal readonly record struct DspTelemetrySnapshot(
    uint Sequence,
    uint ValuesPresent,
    float InputLevelDb,
    float InputLevelFastDb,
    float OutputLevelDb,
    float InputRmsDb,
    float OutputGainDb,
    float OutputGainFastDb,
    float OutputRmsDb,
    float CompressorReductionDb,
    float LimiterReductionDb);

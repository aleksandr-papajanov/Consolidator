namespace Consolidator.Managed.Native;

internal static unsafe class DspTelemetryReader
{
    public static bool TryRead(
        DspStateExchange* exchange,
        out DspTelemetrySnapshot snapshot)
    {
        snapshot = default;
        if (exchange == null)
        {
            return false;
        }

        var sequence = Volatile.Read(ref exchange->Telemetry.Sequence);
        if (sequence == 0 || (sequence & 1U) != 0)
        {
            return false;
        }

        var valuesPresent = exchange->Telemetry.ValuesPresent;
        var inputLevelBits = exchange->Telemetry.InputLevelBits;
        var inputLevelFastBits = exchange->Telemetry.InputLevelFastBits;
        var outputLevelBits = exchange->Telemetry.OutputLevelBits;
        var inputRmsBits = exchange->Telemetry.InputRmsBits;
        var outputGainBits = exchange->Telemetry.OutputGainBits;
        var outputGainFastBits = exchange->Telemetry.OutputGainFastBits;
        var outputRmsBits = exchange->Telemetry.OutputRmsBits;
        var compressorReductionBits = exchange->Telemetry.CompressorReductionBits;
        var limiterReductionBits = exchange->Telemetry.LimiterReductionBits;
        // Complete all payload reads before validating the writer sequence again.
        Thread.MemoryBarrier();
        if (Volatile.Read(ref exchange->Telemetry.Sequence) != sequence)
        {
            return false;
        }

        snapshot = new DspTelemetrySnapshot(
            sequence,
            valuesPresent,
            BitConverter.Int32BitsToSingle((int)inputLevelBits),
            BitConverter.Int32BitsToSingle((int)inputLevelFastBits),
            BitConverter.Int32BitsToSingle((int)outputLevelBits),
            BitConverter.Int32BitsToSingle((int)inputRmsBits),
            BitConverter.Int32BitsToSingle((int)outputGainBits),
            BitConverter.Int32BitsToSingle((int)outputGainFastBits),
            BitConverter.Int32BitsToSingle((int)outputRmsBits),
            BitConverter.Int32BitsToSingle((int)compressorReductionBits),
            BitConverter.Int32BitsToSingle((int)limiterReductionBits));
        return true;
    }
}

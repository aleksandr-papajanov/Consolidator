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
        if (Volatile.Read(ref exchange->Telemetry.Sequence) != sequence)
        {
            return false;
        }

        snapshot = new DspTelemetrySnapshot(
            sequence,
            valuesPresent,
            BitConverter.Int32BitsToSingle((int)inputLevelBits),
            BitConverter.Int32BitsToSingle((int)inputLevelFastBits),
            BitConverter.Int32BitsToSingle((int)outputLevelBits));
        return true;
    }
}

using System.Runtime.InteropServices;

using Consolidator.Managed.Core.Dsp;

namespace Consolidator.Managed.Native;

[Flags]
public enum DspTelemetryValues : uint
{
    None = 0,
    InputLevel = 1U << 0,
    InputLevelFast = 1U << 1,
    OutputLevel = 1U << 2,
    InputRms = 1U << 3,
    OutputGain = 1U << 4,
    OutputGainFast = 1U << 5,
    OutputRms = 1U << 6,
    CompressorReduction = 1U << 7,
    LimiterReduction = 1U << 8
}

[StructLayout(LayoutKind.Sequential)]
public struct DspTelemetryExchange
{
    public uint Sequence;
    public uint ValuesPresent;
    public uint InputLevelBits;
    public uint InputLevelFastBits;
    public uint OutputLevelBits;
    public uint InputRmsBits;
    public uint OutputGainBits;
    public uint OutputGainFastBits;
    public uint OutputRmsBits;
    public uint CompressorReductionBits;
    public uint LimiterReductionBits;
}

[StructLayout(LayoutKind.Sequential)]
public struct DspStateExchange
{
    public DspSnapshot Snapshot0;
    public DspSnapshot Snapshot1;
    public DspSnapshot Snapshot2;
    public uint PublishedIndex;
    public uint ConsumerIndex;
    public DspTelemetryExchange Telemetry;
}

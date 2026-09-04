using System.Runtime.InteropServices;

using Consolidator.Managed.Core.Dsp;

namespace Consolidator.Managed.Native;

[Flags]
public enum DspTelemetryValues : uint
{
    None = 0,
    InputLevel = 1U << 0,
    InputLevelFast = 1U << 1,
    OutputLevel = 1U << 2
}

[StructLayout(LayoutKind.Sequential)]
public struct DspTelemetryExchange
{
    public uint Sequence;
    public uint ValuesPresent;
    public uint InputLevelBits;
    public uint InputLevelFastBits;
    public uint OutputLevelBits;
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

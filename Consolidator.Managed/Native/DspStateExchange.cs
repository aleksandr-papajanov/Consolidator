using System.Runtime.InteropServices;

using Consolidator.Managed.Core.Dsp;

namespace Consolidator.Managed.Native;

[StructLayout(LayoutKind.Sequential)]
public struct DspStateExchange
{
    public DspSnapshot Snapshot0;
    public DspSnapshot Snapshot1;
    public DspSnapshot Snapshot2;
    public uint PublishedIndex;
    public uint ConsumerIndex;
}
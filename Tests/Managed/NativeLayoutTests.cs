using System.Runtime.InteropServices;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Native;
using Xunit;

namespace Consolidator.Managed.Tests;

public sealed class NativeLayoutTests
{
    [Fact]
    public void DspSnapshotLayoutMatchesNativeAbi()
    {
        Assert.Equal(
            4,
            Marshal.SizeOf<DspSnapshot>());
    }

    [Fact]
    public void SharedDspExchangeLayoutMatchesNativeAbi()
    {
        Assert.Equal(
            20,
            Marshal.SizeOf<SharedDspExchange>());
        Assert.Equal(
            0,
            Marshal.OffsetOf<SharedDspExchange>(
                nameof(SharedDspExchange.Snapshot0))
                .ToInt32());
        Assert.Equal(
            4,
            Marshal.OffsetOf<SharedDspExchange>(
                nameof(SharedDspExchange.Snapshot1))
                .ToInt32());
        Assert.Equal(
            8,
            Marshal.OffsetOf<SharedDspExchange>(
                nameof(SharedDspExchange.Snapshot2))
                .ToInt32());
        Assert.Equal(
            12,
            Marshal.OffsetOf<SharedDspExchange>(
                nameof(SharedDspExchange.PublishedIndex))
                .ToInt32());
        Assert.Equal(
            16,
            Marshal.OffsetOf<SharedDspExchange>(
                nameof(SharedDspExchange.ConsumerIndex))
                .ToInt32());
    }
}

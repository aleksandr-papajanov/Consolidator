using System;
using System.Runtime.InteropServices;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Native;
using Xunit;

namespace Consolidator.Managed.Tests.Contracts;

public sealed unsafe class NativeBoundaryContractTests
{
    [Fact]
    public void AbiLayoutAndAtomDecodingMatchTheNativeContract()
    {
        Assert.Equal(16, Marshal.SizeOf<NativeAtom>());
        Assert.Equal(24, Marshal.SizeOf<FilterSnapshot>());
        Assert.Equal(1508, Marshal.SizeOf<DspSnapshot>());
        Assert.Equal(20, Marshal.SizeOf<DspTelemetryExchange>());
        Assert.Equal(4552, Marshal.SizeOf<DspStateExchange>());
        Assert.Equal(
            4524,
            Marshal.OffsetOf<DspStateExchange>(nameof(DspStateExchange.PublishedIndex)).ToInt32());
        Assert.Equal(
            4528,
            Marshal.OffsetOf<DspStateExchange>(nameof(DspStateExchange.ConsumerIndex)).ToInt32());
        Assert.Equal(
            4532,
            Marshal.OffsetOf<DspStateExchange>(nameof(DspStateExchange.Telemetry)).ToInt32());

        var symbol = Marshal.StringToCoTaskMemUTF8("ready");
        try
        {
            var atoms = new NativeAtom[]
            {
                new() { Type = NativeAtomType.Integer, Integer = 42 },
                new() { Type = NativeAtomType.Float, Float = 1.5 },
                new() { Type = NativeAtomType.Symbol, Symbol = (byte*)symbol }
            };
            fixed (NativeAtom* pointer = atoms)
            {
                var decoded = AtomDecoder.Decode(pointer, (nuint)atoms.Length);
                Assert.Equal(42, decoded[0].Integer);
                Assert.Equal(1.5, decoded[1].Float);
                Assert.Equal("ready", decoded[2].Symbol);
            }
        }
        finally
        {
            Marshal.FreeCoTaskMem(symbol);
        }
    }

    [Fact]
    public void InvalidNativeAtomsAreRejectedBeforeProtocolDispatch()
    {
        var atoms = new NativeAtom[]
        {
            new() { Type = (NativeAtomType)99 }
        };

        fixed (NativeAtom* pointer = atoms)
        {
            var rejected = false;
            try
            {
                AtomDecoder.Decode(pointer, 1);
            }
            catch (ArgumentOutOfRangeException)
            {
                rejected = true;
            }

            Assert.True(rejected);
        }
    }

    [Fact]
    public void DspTelemetryReaderAcceptsOnlyAStablePublishedSnapshot()
    {
        var exchange = new DspStateExchange();
        exchange.Telemetry.Sequence = 2;
        exchange.Telemetry.ValuesPresent =
            (uint)(DspTelemetryValues.InputLevel | DspTelemetryValues.InputLevelFast);
        exchange.Telemetry.InputLevelBits = (uint)BitConverter.SingleToInt32Bits(-12.0F);
        exchange.Telemetry.InputLevelFastBits = (uint)BitConverter.SingleToInt32Bits(-6.0F);

        Assert.True(DspTelemetryReader.TryRead(&exchange, out var snapshot));
        Assert.Equal(2U, snapshot.Sequence);
        Assert.Equal(
            (uint)(DspTelemetryValues.InputLevel | DspTelemetryValues.InputLevelFast),
            snapshot.ValuesPresent);
        Assert.Equal(-12.0F, snapshot.InputLevelDb);
        Assert.Equal(-6.0F, snapshot.InputLevelFastDb);

        exchange.Telemetry.Sequence = 3;
        Assert.False(DspTelemetryReader.TryRead(&exchange, out _));
    }
}

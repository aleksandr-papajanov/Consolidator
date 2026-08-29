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
        Assert.Equal(352, Marshal.SizeOf<DspSnapshot>());
        Assert.Equal(1064, Marshal.SizeOf<SharedDspExchange>());
        Assert.Equal(
            1056,
            Marshal.OffsetOf<SharedDspExchange>(nameof(SharedDspExchange.PublishedIndex)).ToInt32());
        Assert.Equal(
            1060,
            Marshal.OffsetOf<SharedDspExchange>(nameof(SharedDspExchange.ConsumerIndex)).ToInt32());

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
}

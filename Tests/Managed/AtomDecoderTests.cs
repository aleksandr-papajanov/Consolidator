using System;
using System.Runtime.InteropServices;
using Consolidator.Managed.Native;
using Consolidator.Managed.Protocol;
using Xunit;

namespace Consolidator.Managed.Tests;

public sealed unsafe class AtomDecoderTests
{
    [Fact]
    public void Decode_MapsIntegerFloatAndUtf8SymbolAtoms()
    {
        var symbol = Marshal.StringToCoTaskMemUTF8("ready");
        var atoms = new NativeAtom[3]
        {
            new() { Type = NativeAtomType.Integer, Integer = 42 },
            new() { Type = NativeAtomType.Float, Float = 1.5 },
            new() { Type = NativeAtomType.Symbol, Symbol = (byte*)symbol }
        };

        try
        {
            fixed (NativeAtom* pointer = atoms)
            {
                var decoded = AtomDecoder.Decode(pointer, (nuint)atoms.Length);

                Assert.Collection(
                    decoded,
                    atom =>
                    {
                        Assert.Equal(AtomType.Integer, atom.Type);
                        Assert.Equal(42L, atom.Integer);
                    },
                    atom =>
                    {
                        Assert.Equal(AtomType.Float, atom.Type);
                        Assert.Equal(1.5, atom.Float);
                    },
                    atom =>
                    {
                        Assert.Equal(AtomType.Symbol, atom.Type);
                        Assert.Equal("ready", atom.Symbol);
                    });
            }
        }
        finally
        {
            Marshal.FreeCoTaskMem(symbol);
        }
    }

    [Fact]
    public void Decode_RejectsUnknownAtomType()
    {
        var atoms = new NativeAtom[1]
        {
            new() { Type = (NativeAtomType)99 }
        };

        fixed (NativeAtom* pointer = atoms)
        {
            try
            {
                AtomDecoder.Decode(pointer, 1);
                Assert.Fail("Unknown atom type should be rejected.");
            }
            catch (ArgumentOutOfRangeException)
            {
            }
        }
    }
}

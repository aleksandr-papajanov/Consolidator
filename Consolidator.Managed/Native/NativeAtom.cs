using System.Runtime.InteropServices;

namespace Consolidator.Managed.Native;

public enum NativeAtomType : byte
{
    Integer = 1,
    Float = 2,
    Symbol = 3
}

[StructLayout(LayoutKind.Explicit, Size = 16)]
public unsafe struct NativeAtom
{
    [FieldOffset(0)]
    public NativeAtomType Type;

    [FieldOffset(8)]
    public long Integer;

    [FieldOffset(8)]
    public double Float;

    [FieldOffset(8)]
    public byte* Symbol;
}
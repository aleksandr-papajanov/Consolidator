using System.Runtime.InteropServices;
using Consolidator.Managed.Core.Abstractions;
using Consolidator.Managed.Protocol;

namespace Consolidator.Managed.Native;

public unsafe sealed class NativeOutput : IInstanceOutput
{
    private readonly void* _context;
    private readonly delegate* unmanaged[Cdecl]<
        void*,
        byte*,
        NativeAtom*,
        nuint,
        void> _callback;

    public NativeOutput(
        void* context,
        delegate* unmanaged[Cdecl]<
            void*,
            byte*,
            NativeAtom*,
            nuint,
            void> callback)
    {
        if (callback == null)
        {
            throw new ArgumentNullException(nameof(callback));
        }

        _context = context;
        _callback = callback;
    }

    public void Send(
        string selector,
        ReadOnlySpan<Atom> atoms)
    {
        ArgumentNullException.ThrowIfNull(selector);

        var selectorPointer = Marshal.StringToCoTaskMemUTF8(selector);
        var nativeAtoms = new NativeAtom[atoms.Length];
        var symbolPointers = new nint[atoms.Length];

        try
        {
            for (var index = 0; index < atoms.Length; index++)
            {
                nativeAtoms[index] = atoms[index].Type switch
                {
                    AtomType.Integer => new NativeAtom
                    {
                        Type = NativeAtomType.Integer,
                        Integer = atoms[index].Integer
                    },
                    AtomType.Float => new NativeAtom
                    {
                        Type = NativeAtomType.Float,
                        Float = atoms[index].Float
                    },
                    AtomType.Symbol => CreateSymbolAtom(
                        atoms[index].Symbol,
                        symbolPointers,
                        index),
                    _ => throw new ArgumentOutOfRangeException()
                };
            }

            fixed (NativeAtom* atomPointer = nativeAtoms)
            {
                _callback(
                    _context,
                    (byte*)selectorPointer,
                    atomPointer,
                    (nuint)nativeAtoms.Length);
            }
        }
        finally
        {
            Marshal.FreeCoTaskMem(selectorPointer);

            foreach (var symbolPointer in symbolPointers)
            {
                if (symbolPointer != 0)
                {
                    Marshal.FreeCoTaskMem(symbolPointer);
                }
            }
        }
    }

    private static NativeAtom CreateSymbolAtom(
        string? value,
        nint[] symbolPointers,
        int index)
    {
        var pointer = Marshal.StringToCoTaskMemUTF8(value ?? string.Empty);
        symbolPointers[index] = pointer;

        return new NativeAtom
        {
            Type = NativeAtomType.Symbol,
            Symbol = (byte*)pointer
        };
    }
}
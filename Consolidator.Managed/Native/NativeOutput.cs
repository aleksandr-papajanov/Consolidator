using System.Buffers;
using System.Runtime.InteropServices;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Native;

internal unsafe sealed class NativeOutput : IProtocolOutputCallback
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

    public void Send(ProtocolOutput message)
    {
        ArgumentNullException.ThrowIfNull(message);

        var selectorPointer = Marshal.StringToCoTaskMemUTF8(message.Selector);
        var nativeAtoms = ArrayPool<NativeAtom>.Shared.Rent(message.Atoms.Count);
        var hasSymbols = false;
        for (var index = 0; index < message.Atoms.Count; index++)
        {
            if (message.Atoms[index].Type == AtomType.Symbol)
            {
                hasSymbols = true;
                break;
            }
        }
        var symbolPointers = hasSymbols
            ? ArrayPool<nint>.Shared.Rent(message.Atoms.Count)
            : null;
        if (symbolPointers is not null)
        {
            Array.Clear(symbolPointers, 0, message.Atoms.Count);
        }

        try
        {
            for (var index = 0; index < message.Atoms.Count; index++)
            {
                nativeAtoms[index] = message.Atoms[index].Type switch
                {
                    AtomType.Integer => new NativeAtom
                    {
                        Type = NativeAtomType.Integer,
                        Integer = message.Atoms[index].Integer
                    },
                    AtomType.Float => new NativeAtom
                    {
                        Type = NativeAtomType.Float,
                        Float = message.Atoms[index].Float
                    },
                    AtomType.Symbol => CreateSymbolAtom(
                        message.Atoms[index].Symbol,
                        symbolPointers!,
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
                    (nuint)message.Atoms.Count);
            }
        }
        finally
        {
            Marshal.FreeCoTaskMem(selectorPointer);

            if (symbolPointers is not null)
            {
                for (var index = 0; index < message.Atoms.Count; index++)
                {
                    if (symbolPointers[index] != 0)
                    {
                        Marshal.FreeCoTaskMem(symbolPointers[index]);
                    }
                }
                ArrayPool<nint>.Shared.Return(symbolPointers);
            }
            ArrayPool<NativeAtom>.Shared.Return(nativeAtoms);
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




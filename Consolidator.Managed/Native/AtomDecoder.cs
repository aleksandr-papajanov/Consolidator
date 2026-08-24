using System.Runtime.InteropServices;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Native;

public static unsafe class AtomDecoder
{
    public static Atom[] Decode(
        NativeAtom* atoms,
        nuint count)
    {
        var result = new Atom[(int)count];

        for (var index = 0; index < (int)count; index++)
        {
            var atom = atoms[index];

            result[index] = atom.Type switch
            {
                NativeAtomType.Integer =>
                    new Atom(
                        AtomType.Integer,
                        atom.Integer,
                        0,
                        null),

                NativeAtomType.Float =>
                    new Atom(
                        AtomType.Float,
                        0,
                        atom.Float,
                        null),

                NativeAtomType.Symbol =>
                    new Atom(
                        AtomType.Symbol,
                        0,
                        0,
                        Marshal.PtrToStringUTF8(
                            (nint)atom.Symbol)),

                _ => throw new ArgumentOutOfRangeException()
            };
        }

        return result;
    }
}




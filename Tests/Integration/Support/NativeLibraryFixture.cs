using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using Xunit;

namespace Consolidator.Integration.Tests.Support;

public sealed class NativeLibraryFixture : IDisposable
{
    public static readonly string PublishedLibraryPath = Path.GetFullPath(
        Path.Combine(
            AppContext.BaseDirectory,
            "..",
            "..",
            "..",
            "..",
            "..",
            "Consolidator.Max",
            "externals",
            "Consolidator.Managed.dll"));

    private readonly nint _library;
    private readonly OutputCallback _outputCallback;
    private readonly RegisterInstance _registerInstance;
    private readonly UnregisterInstance _unregisterInstance;
    private readonly SendMessage _sendMessage;
    private readonly CapturePersistenceExport _capturePersistence;
    private readonly FreePersistence _freePersistence;
    private readonly RestorePersistenceExport _restorePersistence;
    private readonly Shutdown _shutdown;
    private bool _disposed;

    public NativeLibraryFixture()
    {
        if (!File.Exists(PublishedLibraryPath))
        {
            throw new FileNotFoundException(
                "Publish the NativeAOT library before integration tests.",
                PublishedLibraryPath);
        }

        _library = NativeLibrary.Load(PublishedLibraryPath);
        _outputCallback = ReceiveOutput;
        _registerInstance = Load<RegisterInstance>("ConsolidatorRegisterInstance");
        _unregisterInstance = Load<UnregisterInstance>("ConsolidatorUnregisterInstance");
        _sendMessage = Load<SendMessage>("ConsolidatorSendMessage");
        _capturePersistence = Load<CapturePersistenceExport>(
            "ConsolidatorCapturePersistence");
        _freePersistence = Load<FreePersistence>(
            "ConsolidatorFreePersistence");
        _restorePersistence = Load<RestorePersistenceExport>(
            "ConsolidatorRestorePersistence");
        _shutdown = Load<Shutdown>("ConsolidatorShutdown");
    }

    public nint Library => _library;

    public NativeInstance Register()
    {
        var instance = new NativeInstance(this);
        var context = GCHandle.Alloc(instance);
        var exchange = Marshal.AllocHGlobal(1064);
        Span<byte> zero = stackalloc byte[1064];
        Marshal.Copy(zero.ToArray(), 0, exchange, zero.Length);
        var instanceId = _registerInstance(
            GCHandle.ToIntPtr(context),
            Marshal.GetFunctionPointerForDelegate(_outputCallback),
            exchange,
            out var audioInputHandle);
        if (instanceId == 0)
        {
            context.Free();
            Marshal.FreeHGlobal(exchange);
            throw new InvalidOperationException("NativeAOT instance registration failed.");
        }

        instance.Initialize(instanceId, audioInputHandle, context, exchange);
        return instance;
    }

    public void Send(
        NativeInstance instance,
        string selector,
        params InputAtom[] atoms)
    {
        var selectorPointer = Marshal.StringToCoTaskMemUTF8(selector);
        var atomPointer = atoms.Length == 0
            ? nint.Zero
            : Marshal.AllocHGlobal(atoms.Length * NativeAtom.Size);
        var symbolPointers = new List<nint>();
        try
        {
            for (var index = 0; index < atoms.Length; index++)
            {
                var atom = atoms[index].ToNative(symbolPointers);
                Marshal.StructureToPtr(
                    atom,
                    atomPointer + index * NativeAtom.Size,
                    false);
            }

            _sendMessage(
                instance.InstanceId,
                selectorPointer,
                atomPointer,
                (nuint)atoms.Length);
        }
        finally
        {
            foreach (var pointer in symbolPointers)
            {
                Marshal.FreeCoTaskMem(pointer);
            }

            if (atomPointer != nint.Zero)
            {
                Marshal.FreeHGlobal(atomPointer);
            }

            Marshal.FreeCoTaskMem(selectorPointer);
        }
    }

    public TDelegate Load<TDelegate>(string export)
        where TDelegate : Delegate
    {
        var pointer = NativeLibrary.GetExport(_library, export);
        return Marshal.GetDelegateForFunctionPointer<TDelegate>(pointer);
    }

    public byte[] CapturePersistence(NativeInstance instance)
    {
        if (_capturePersistence(
                instance.InstanceId,
                out var data,
                out var length) == 0)
        {
            throw new InvalidOperationException(
                "NativeAOT persistence capture failed.");
        }

        try
        {
            if (data == nint.Zero || length == 0 || length > int.MaxValue)
            {
                throw new InvalidOperationException(
                    "NativeAOT persistence capture returned an invalid buffer.");
            }

            var payload = new byte[(int)length];
            Marshal.Copy(data, payload, 0, payload.Length);
            return payload;
        }
        finally
        {
            _freePersistence(data);
        }
    }

    public bool RestorePersistence(
        NativeInstance instance,
        byte[] payload)
    {
        ArgumentNullException.ThrowIfNull(payload);
        var data = Marshal.AllocHGlobal(payload.Length);
        try
        {
            Marshal.Copy(payload, 0, data, payload.Length);
            return _restorePersistence(
                instance.InstanceId,
                data,
                (nuint)payload.Length) != 0;
        }
        finally
        {
            Marshal.FreeHGlobal(data);
        }
    }

    public void ShutdownServices()
    {
        _shutdown();
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        _shutdown();
    }

    internal void Unregister(NativeInstance instance)
    {
        _unregisterInstance(instance.InstanceId);
    }

    private static void ReceiveOutput(
        nint context,
        nint selector,
        nint atoms,
        nuint atomCount)
    {
        var handle = GCHandle.FromIntPtr(context);
        if (handle.Target is not NativeInstance instance)
        {
            return;
        }

        var values = new OutputAtom[(int)atomCount];
        for (var index = 0; index < values.Length; index++)
        {
            var atom = Marshal.PtrToStructure<NativeAtom>(
                atoms + index * NativeAtom.Size);
            values[index] = atom.Type switch
            {
                NativeAtomType.Integer => OutputAtom.Integer(atom.Integer),
                NativeAtomType.Float => OutputAtom.Float(atom.Float),
                NativeAtomType.Symbol => OutputAtom.Symbol(
                    Marshal.PtrToStringUTF8(atom.Symbol) ?? string.Empty),
                _ => throw new InvalidOperationException(
                    $"Unknown callback atom type: {atom.Type}.")
            };
        }

        instance.AddFrame(new OutputFrame(
            Marshal.PtrToStringUTF8(selector) ?? string.Empty,
            values));
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate ulong RegisterInstance(
        nint context,
        nint outputCallback,
        nint dspExchange,
        out nuint audioInputHandle);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void UnregisterInstance(ulong instanceId);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void SendMessage(
        ulong instanceId,
        nint selector,
        nint atoms,
        nuint atomCount);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int CapturePersistenceExport(
        ulong instanceId,
        out nint data,
        out nuint length);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void FreePersistence(nint data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int RestorePersistenceExport(
        ulong instanceId,
        nint data,
        nuint length);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void Shutdown();

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void OutputCallback(
        nint context,
        nint selector,
        nint atoms,
        nuint atomCount);
}

public sealed class NativeInstance : IDisposable
{
    private readonly NativeLibraryFixture _library;
    private GCHandle _context;
    private nint _exchange;
    private readonly List<OutputFrame> _frames = new();
    private readonly object _frameLock = new();
    private bool _disposed;

    internal NativeInstance(NativeLibraryFixture library)
    {
        _library = library;
    }

    public ulong InstanceId { get; private set; }

    public nuint AudioInputHandle { get; private set; }

    public IReadOnlyList<OutputFrame> Frames
    {
        get
        {
            lock (_frameLock)
            {
                return _frames.ToArray();
            }
        }
    }

    public int PublishedSnapshotIndex => Marshal.ReadInt32(_exchange, 1056);

    public float PublishedGain => Marshal.PtrToStructure<float>(
        _exchange + PublishedSnapshotIndex * 352);

    internal void Initialize(
        ulong instanceId,
        nuint audioInputHandle,
        GCHandle context,
        nint exchange)
    {
        InstanceId = instanceId;
        AudioInputHandle = audioInputHandle;
        _context = context;
        _exchange = exchange;
    }

    public OutputFrame Single(string selector)
    {
        lock (_frameLock)
        {
            return Assert.Single(
                _frames,
                frame => frame.Selector == selector);
        }
    }

    public void ClearFrames()
    {
        lock (_frameLock)
        {
            _frames.Clear();
        }
    }

    public void WaitForResponse(string requestId)
    {
        Assert.True(SpinWait.SpinUntil(
            () => HasResponse(requestId),
            TimeSpan.FromSeconds(5)),
            $"No terminal callback was received for request {requestId}.");
    }

    internal void AddFrame(OutputFrame frame)
    {
        lock (_frameLock)
        {
            _frames.Add(frame);
        }
    }

    private bool HasResponse(string requestId)
    {
        lock (_frameLock)
        {
            return _frames.Any(frame =>
                (frame.Selector is "action_done" or "error" or "state_done" or
                    "target_state_snapshot" or "registry_done") &&
                frame.Atoms.Count > 2 &&
                frame.Atoms[2].SymbolValue == requestId);
        }
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        _library.Unregister(this);
        _context.Free();
        Marshal.FreeHGlobal(_exchange);
    }
}

public sealed record OutputFrame(
    string Selector,
    IReadOnlyList<OutputAtom> Atoms);

public sealed record OutputAtom(
    NativeAtomType Type,
    long IntegerValue,
    double FloatValue,
    string? SymbolValue)
{
    public static OutputAtom Integer(long value) =>
        new(NativeAtomType.Integer, value, 0, null);

    public static OutputAtom Float(double value) =>
        new(NativeAtomType.Float, 0, value, null);

    public static OutputAtom Symbol(string value) =>
        new(NativeAtomType.Symbol, 0, 0, value);
}

public sealed record InputAtom(
    NativeAtomType Type,
    long IntegerValue,
    double FloatValue,
    string? SymbolValue)
{
    public static InputAtom Integer(long value) =>
        new(NativeAtomType.Integer, value, 0, null);

    public static InputAtom Float(double value) =>
        new(NativeAtomType.Float, 0, value, null);

    public static InputAtom Symbol(string value) =>
        new(NativeAtomType.Symbol, 0, 0, value);

    internal NativeAtom ToNative(List<nint> symbolPointers)
    {
        var atom = new NativeAtom { Type = Type };
        switch (Type)
        {
            case NativeAtomType.Integer:
                atom.Integer = IntegerValue;
                break;
            case NativeAtomType.Float:
                atom.Float = FloatValue;
                break;
            case NativeAtomType.Symbol:
                var pointer = Marshal.StringToCoTaskMemUTF8(SymbolValue);
                symbolPointers.Add(pointer);
                atom.Symbol = pointer;
                break;
            default:
                throw new ArgumentOutOfRangeException(nameof(Type));
        }

        return atom;
    }
}

public enum NativeAtomType : byte
{
    Integer = 1,
    Float = 2,
    Symbol = 3
}

[StructLayout(LayoutKind.Explicit, Size = Size)]
internal struct NativeAtom
{
    public const int Size = 16;

    [FieldOffset(0)]
    public NativeAtomType Type;

    [FieldOffset(8)]
    public long Integer;

    [FieldOffset(8)]
    public double Float;

    [FieldOffset(8)]
    public nint Symbol;
}

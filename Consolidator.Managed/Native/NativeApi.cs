using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using Microsoft.Extensions.DependencyInjection;
using Consolidator.Managed.Core;
using Consolidator.Managed.Protocol;

namespace Consolidator.Managed.Native;

public static unsafe class NativeApi
{
    private static readonly NativeLogSink LogSink =
        ManagedServices.Provider.GetRequiredService<NativeLogSink>();
    private static readonly ConsolidatorCore Core =
        ManagedServices.Provider.GetRequiredService<ConsolidatorCore>();
    private static long _audioBoundaryExceptionCount;

    [UnmanagedCallersOnly(
        EntryPoint = "ConsolidatorSetLogCallback",
        CallConvs = [typeof(CallConvCdecl)])]
    public static void SetLogCallback(
        void* context,
        delegate* unmanaged[Cdecl]<void*, byte*, void> callback)
    {
        try
        {
            LogSink.Configure(context, callback);
        }
        catch (Exception exception)
        {
            LogBoundaryException(
                "ConsolidatorSetLogCallback",
                exception);
        }
    }

    [UnmanagedCallersOnly(
        EntryPoint = "ConsolidatorRegisterInstance",
        CallConvs = [typeof(CallConvCdecl)])]
    public static ulong RegisterInstance(
        void* context,
        delegate* unmanaged[Cdecl]<
            void*,
            byte*,
            NativeAtom*,
            nuint,
            void> outputCallback,
        SharedDspExchange* dspExchange)
    {
        try
        {
            if (outputCallback == null)
            {
                return 0;
            }

            if (dspExchange == null)
            {
                return 0;
            }

            var output = new NativeOutput(context, outputCallback);
            var publisher = new NativeDspStatePublisher(dspExchange);
            return Core.RegisterInstance(output, publisher);
        }
        catch (Exception exception)
        {
            LogBoundaryException(
                "ConsolidatorRegisterInstance",
                exception);
            return 0;
        }
    }

    [UnmanagedCallersOnly(
        EntryPoint = "ConsolidatorUnregisterInstance",
        CallConvs = [typeof(CallConvCdecl)])]
    public static void UnregisterInstance(ulong instanceId)
    {
        try
        {
            ReportPendingAudioBoundaryExceptions();
            Core.UnregisterInstance(instanceId);
        }
        catch (Exception exception)
        {
            LogBoundaryException(
                "ConsolidatorUnregisterInstance",
                exception);
        }
    }

    [UnmanagedCallersOnly(
        EntryPoint = "ConsolidatorSendMessage",
        CallConvs = [typeof(CallConvCdecl)])]
    public static void SendMessage(
        ulong instanceId,
        byte* selector,
        NativeAtom* atoms,
        nuint atomCount)
    {
        try
        {
            ReportPendingAudioBoundaryExceptions();

            var managedSelector = Marshal.PtrToStringUTF8((nint)selector);

            if (managedSelector is null)
            {
                return;
            }

            var managedAtoms = AtomDecoder.Decode(atoms, atomCount);

            var atomText = string.Join(
                " ",
                managedAtoms.Select(FormatAtom));

            LogSink.Write(
                $"Managed received: instance={instanceId} "
                + $"selector={managedSelector} atoms=[{atomText}]");

            Core.ReceiveMessage(instanceId, managedSelector, managedAtoms);
        }
        catch (Exception exception)
        {
            LogBoundaryException(
                "ConsolidatorSendMessage",
                exception);
        }
    }

    private static string FormatAtom(Atom atom)
    {
        return atom.Type switch
        {
            AtomType.Integer => $"int:{atom.Integer}",
            AtomType.Float => $"float:{atom.Float}",
            AtomType.Symbol => $"symbol:{atom.Symbol}",
            _ => $"unknown:{atom.Type}"
        };
    }

    [UnmanagedCallersOnly(
        EntryPoint = "ConsolidatorPrepare",
        CallConvs = [typeof(CallConvCdecl)])]
    public static void Prepare(
        ulong instanceId,
        double sampleRate,
        nuint maximumFrameCount)
    {
        try
        {
            ReportPendingAudioBoundaryExceptions();

            Core.Prepare(
                instanceId,
                sampleRate,
                maximumFrameCount);
        }
        catch (Exception exception)
        {
            LogBoundaryException(
                "ConsolidatorPrepare",
                exception);
        }
    }

    [UnmanagedCallersOnly(
        EntryPoint = "ConsolidatorSendAudio",
        CallConvs = [typeof(CallConvCdecl)])]
    public static void SendAudio(
        ulong instanceId,
        double* mainLeft,
        double* mainRight,
        double* referenceLeft,
        double* referenceRight,
        nuint frameCount)
    {
        try
        {
            Core.ReceiveAudio(
                instanceId,
                mainLeft,
                mainRight,
                referenceLeft,
                referenceRight,
                frameCount);
        }
        catch
        {
            Interlocked.Increment(ref _audioBoundaryExceptionCount);
        }
    }

    private static void ReportPendingAudioBoundaryExceptions()
    {
        var exceptionCount =
            Interlocked.Exchange(
                ref _audioBoundaryExceptionCount,
                0);

        if (exceptionCount == 0)
        {
            return;
        }

        try
        {
            LogSink.Write(
                $"Managed audio boundary exceptions: {exceptionCount}");
        }
        catch
        {
        }
    }

    private static void LogBoundaryException(
        string entryPoint,
        Exception exception)
    {
        try
        {
            LogSink.Write(
                $"Managed boundary exception in {entryPoint}: "
                + exception);
        }
        catch
        {
        }
    }
}
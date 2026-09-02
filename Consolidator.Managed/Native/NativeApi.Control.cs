using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Protocol.Messages;

namespace Consolidator.Managed.Native;

public static unsafe partial class NativeApi
{
    [UnmanagedCallersOnly(
        EntryPoint = "ConsolidatorSendMessage",
        CallConvs = [typeof(CallConvCdecl)])]
    public static void SendMessage(
        ulong instanceId,
        byte* selector,
        NativeAtom* atoms,
        nuint atomCount)
    {
        var startedAt = Stopwatch.GetTimestamp();

        try
        {
            ReportPendingAudioBoundaryExceptions();

            var managedSelector = Marshal.PtrToStringUTF8((nint)selector);

            if (managedSelector is null)
            {
                return;
            }

            if (managedSelector == "metrics")
            {
                LogSink.Write(RuntimeMetrics.Shared.FormatSnapshot());
                return;
            }

            var managedAtoms = AtomDecoder.Decode(atoms, atomCount);

            ProtocolService.Receive(
                new ProtocolInput(
                    instanceId,
                    managedSelector,
                    managedAtoms));
        }
        catch (Exception exception)
        {
            LogBoundaryException(
                "ConsolidatorSendMessage",
                exception);
        }
        finally
        {
            RuntimeMetrics.Shared.RecordNativeInput(
                Stopwatch.GetTimestamp() - startedAt);
        }
    }
}
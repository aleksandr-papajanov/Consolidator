using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Consolidator.Managed.Native;

public static unsafe partial class NativeApi
{
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
            PreparationService.Prepare(
                new InstanceId(instanceId),
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
        nuint audioInputHandle,
        double* mainLeft,
        double* mainRight,
        double* referenceLeft,
        double* referenceRight,
        nuint frameCount)
    {
        try
        {
            if (audioInputHandle == 0)
            {
                return;
            }

            var audioInput = (NativeAudioInput?)GCHandle
                .FromIntPtr((nint)audioInputHandle)
                .Target;

            if (audioInput is null)
            {
                return;
            }

            audioInput.ReceiveAudio(
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
}
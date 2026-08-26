using System.Collections.Concurrent;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using Microsoft.Extensions.DependencyInjection;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Protocol;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;
using Consolidator.Managed.Services;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Analyzer;

namespace Consolidator.Managed.Native;

public static unsafe class NativeApi
{
    private static readonly NativeLogSink LogSink = NativeLogSink.Shared;

    private static IInstanceLifecycleService LifecycleService =>
        ManagedServices.Provider.GetRequiredService<IInstanceLifecycleService>();

    private static IProtocolOutputRegistry OutputRegistry =>
        ManagedServices.Provider.GetRequiredService<IProtocolOutputRegistry>();

    private static ProtocolService ProtocolService =>
        ManagedServices.Provider.GetRequiredService<ProtocolService>();

    private static IInstancePreparationService PreparationService =>
        ManagedServices.Provider.GetRequiredService<IInstancePreparationService>();

    private static IInstanceAudioInputService AudioInputService =>
        ManagedServices.Provider.GetRequiredService<IInstanceAudioInputService>();

    private static FftAnalyzer FftAnalyzer =>
        ManagedServices.Provider.GetRequiredService<FftAnalyzer>();

    private static InstanceActivityCoordinator ActivityCoordinator =>
        ManagedServices.Provider.GetRequiredService<InstanceActivityCoordinator>();
    private static readonly ConcurrentDictionary<ulong, GCHandle>
        AudioInputHandles = new();
    private static long _audioBoundaryExceptionCount;
    private static int _shutdownStarted;

    [UnmanagedCallersOnly(
        EntryPoint = "ConsolidatorShutdown",
        CallConvs = [typeof(CallConvCdecl)])]
    public static void Shutdown()
    {
        if (Interlocked.Exchange(ref _shutdownStarted, 1) != 0)
        {
            return;
        }

        try
        {
            ManagedServices.Dispose();
        }
        catch (Exception exception)
        {
            LogBoundaryException(
                "ConsolidatorShutdown",
                exception);
        }
        finally
        {
            foreach (var entry in AudioInputHandles.ToArray())
            {
                if (AudioInputHandles.TryRemove(entry.Key, out var handle))
                {
                    handle.Free();
                }
            }
        }
    }

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
        SharedDspExchange* dspExchange,
        nuint* audioInputHandle)
    {
        InstanceId instanceId = default;
        GCHandle handle = default;

        try
        {
            Interlocked.Exchange(ref _shutdownStarted, 0);
            if (outputCallback == null)
            {
                LogSink.Write(
                    "ConsolidatorRegisterInstance rejected a null output callback.");
                return 0;
            }

            if (dspExchange == null)
            {
                LogSink.Write(
                    "ConsolidatorRegisterInstance rejected a null DSP exchange.");
                return 0;
            }

            if (audioInputHandle == null)
            {
                LogSink.Write(
                    "ConsolidatorRegisterInstance rejected a null audio input handle.");
                return 0;
            }

            *audioInputHandle = 0;

            var publisher = new NativeDspStatePublisher(dspExchange);
            instanceId = LifecycleService.RegisterInstance(publisher);
            OutputRegistry.Register(
                instanceId.Value,
                new NativeOutput(context, outputCallback));
            FftAnalyzer.ReplayEqualizerPresentation(instanceId);

            var audioInput = new NativeAudioInput(instanceId, AudioInputService);
            handle = GCHandle.Alloc(audioInput);

            if (!AudioInputHandles.TryAdd(instanceId.Value, handle))
            {
                LogSink.Write(
                    "ConsolidatorRegisterInstance could not register "
                    + $"the audio input handle for instance {instanceId.Value}.");
                handle.Free();
                handle = default;
                ActivityCoordinator.Unregister(instanceId.Value);
                OutputRegistry.Unregister(instanceId.Value);
                LifecycleService.UnregisterInstance(instanceId);
                return 0;
            }

            *audioInputHandle = (nuint)GCHandle.ToIntPtr(handle);
            handle = default;
            return instanceId.Value;
        }
        catch (Exception exception)
        {
            if (audioInputHandle != null)
            {
                *audioInputHandle = 0;
            }

            LogBoundaryException(
                "ConsolidatorRegisterInstance",
                exception);

            try
            {
                if (instanceId.IsValid)
                {
                    ActivityCoordinator.Unregister(instanceId.Value);
                    OutputRegistry.Unregister(instanceId.Value);
                    LifecycleService.UnregisterInstance(instanceId);
                }

                if (instanceId.IsValid &&
                    AudioInputHandles.TryRemove(
                        instanceId.Value,
                        out var registeredHandle))
                {
                    registeredHandle.Free();
                }
                else if (handle.IsAllocated)
                {
                    handle.Free();
                }
            }
            catch (Exception cleanupException)
            {
                LogBoundaryException(
                    "ConsolidatorRegisterInstance cleanup",
                    cleanupException);
            }

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
            ProtocolService.CancelInstance(instanceId);
            ActivityCoordinator.Unregister(instanceId);
            OutputRegistry.Unregister(instanceId);
            LifecycleService.UnregisterInstance(new InstanceId(instanceId));

            if (AudioInputHandles.TryRemove(
                instanceId,
                out var audioInputHandle))
            {
                audioInputHandle.Free();
            }
        }
        catch (Exception exception)
        {
            LogBoundaryException(
                "ConsolidatorUnregisterInstance",
                exception);
        }
        finally
        {
            if (AudioInputHandles.TryRemove(
                instanceId,
                out var audioInputHandle))
            {
                audioInputHandle.Free();
            }
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




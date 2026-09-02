using System.Collections.Concurrent;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.Persistence;
using Consolidator.Managed.Protocol;
using Consolidator.Managed.Protocol.Transport;
using Consolidator.Managed.Composition;
using Microsoft.Extensions.DependencyInjection;

namespace Consolidator.Managed.Native;

public static unsafe partial class NativeApi
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

    private static ActiveInstanceCoordinator ActivityCoordinator =>
        ManagedServices.Provider.GetRequiredService<ActiveInstanceCoordinator>();

    private static InstancePersistenceService PersistenceService =>
        ManagedServices.Provider.GetRequiredService<InstancePersistenceService>();

    private static readonly ConcurrentDictionary<ulong, GCHandle>
        AudioInputHandles = new();
    private static long _audioBoundaryExceptionCount;
    private static int _shutdownStarted;

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
        DspStateExchange* dspExchange,
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

}




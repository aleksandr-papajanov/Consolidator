using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Consolidator.Managed.Composition;

namespace Consolidator.Managed.Native;

public static unsafe partial class NativeApi
{
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
}
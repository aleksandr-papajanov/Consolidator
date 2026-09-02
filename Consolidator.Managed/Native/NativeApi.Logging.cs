using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Consolidator.Managed.Native;

public static unsafe partial class NativeApi
{
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
}
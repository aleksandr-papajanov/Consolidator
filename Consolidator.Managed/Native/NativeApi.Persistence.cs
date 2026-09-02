using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Consolidator.Managed.Native;

public static unsafe partial class NativeApi
{
    [UnmanagedCallersOnly(
        EntryPoint = "ConsolidatorCapturePersistence",
        CallConvs = [typeof(CallConvCdecl)])]
    public static int CapturePersistence(
        ulong instanceId,
        byte** data,
        nuint* length)
    {
        try
        {
            if (data == null || length == null)
            {
                return 0;
            }

            *data = null;
            *length = 0;
            var utf8 = ProtocolService.ExecuteControlBarrier(
                () => PersistenceService.CaptureCommitted(new InstanceId(instanceId)));
            var pointer = (byte*)Marshal.AllocCoTaskMem(utf8.Length);
            try
            {
                Marshal.Copy(utf8, 0, (nint)pointer, utf8.Length);
            }
            catch
            {
                Marshal.FreeCoTaskMem((nint)pointer);
                throw;
            }
            *data = pointer;
            *length = (nuint)utf8.Length;
            return 1;
        }
        catch (Exception exception)
        {
            LogBoundaryException("ConsolidatorCapturePersistence", exception);
            return 0;
        }
    }

    [UnmanagedCallersOnly(
        EntryPoint = "ConsolidatorFreePersistence",
        CallConvs = [typeof(CallConvCdecl)])]
    public static void FreePersistence(byte* data)
    {
        if (data != null)
        {
            Marshal.FreeCoTaskMem((nint)data);
        }
    }

    [UnmanagedCallersOnly(
        EntryPoint = "ConsolidatorRestorePersistence",
        CallConvs = [typeof(CallConvCdecl)])]
    public static int RestorePersistence(
        ulong instanceId,
        byte* data,
        nuint length)
    {
        try
        {
            if (data == null || length > int.MaxValue)
            {
                return 0;
            }

            var payload = new byte[(int)length];
            Marshal.Copy((nint)data, payload, 0, payload.Length);
            ProtocolService.ExecuteControlBarrier(() =>
            {
                PersistenceService.Restore(
                    new InstanceId(instanceId),
                    payload);
                return true;
            });
            return 1;
        }
        catch (Exception exception)
        {
            LogBoundaryException("ConsolidatorRestorePersistence", exception);
            return 0;
        }
    }
}
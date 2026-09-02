namespace Consolidator.Managed.Native;

public static unsafe partial class NativeApi
{
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
using Consolidator.Managed.Core.Services.Abstractions;

namespace Consolidator.Managed.Native;

public sealed class NativeLogService : IManagedLogger
{
    private readonly NativeLogSink _sink;

    public NativeLogService(NativeLogSink sink)
    {
        _sink = sink;
    }

    public void Info(string message)
    {
        _sink.Write($"[INFO] {message}");
    }

    public void Warning(string message)
    {
        _sink.Write($"[WARN] {message}");
    }

    public void Error(string message)
    {
        _sink.Write($"[ERROR] {message}");
    }
}





using Consolidator.Managed.Core;

namespace Consolidator.Managed.Native;

public sealed class ConsolidatorLogger : IConsolidatorLogger
{
    private readonly NativeLogSink _sink;

    public ConsolidatorLogger(NativeLogSink sink)
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

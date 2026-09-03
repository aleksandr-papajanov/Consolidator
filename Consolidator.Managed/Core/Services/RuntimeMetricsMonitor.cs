using System.Threading;

using Consolidator.Managed.Core.Services.Abstractions;

namespace Consolidator.Managed.Core.Services;

public sealed class RuntimeMetricsMonitor : IDisposable
{
    private static readonly TimeSpan SampleInterval = TimeSpan.FromSeconds(5);
    private static readonly TimeSpan AlertCooldown = TimeSpan.FromSeconds(30);
    private const long MinimumSampleCount = 4;
    private const double SlowInputMilliseconds = 5;
    private const double SlowControlMilliseconds = 5;

    private readonly RuntimeMetrics _metrics;
    private readonly IManagedLogger _logger;
    private readonly Timer _timer;
    private readonly object _sync = new();
    private MetricsSample? _previous;
    private DateTimeOffset _lastInputAlert;
    private DateTimeOffset _lastControlAlert;
    private DateTimeOffset _lastAudioDropAlert;
    private int _disposed;

    public RuntimeMetricsMonitor(
        RuntimeMetrics metrics,
        IManagedLogger logger)
    {
        ArgumentNullException.ThrowIfNull(metrics);
        ArgumentNullException.ThrowIfNull(logger);

        _metrics = metrics;
        _logger = logger;
        _timer = new Timer(
            Check,
            null,
            SampleInterval,
            SampleInterval);
    }

    public void Dispose()
    {
        if (Interlocked.Exchange(ref _disposed, 1) != 0)
        {
            return;
        }

        _timer.Dispose();
    }

    private void Check(object? state)
    {
        if (Volatile.Read(ref _disposed) != 0)
        {
            return;
        }

        try
        {
            var current = _metrics.CaptureSample();
            lock (_sync)
            {
                if (_previous is null)
                {
                    _previous = current;
                    return;
                }

                var previous = _previous;
                _previous = current;
                Evaluate(current, previous);
            }
        }
        catch (Exception exception)
        {
            _logger.Error($"Runtime metrics monitor failed: {exception.Message}");
        }
    }

    private void Evaluate(
        MetricsSample current,
        MetricsSample previous)
    {
        var now = DateTimeOffset.UtcNow;
        var inputCalls = current.NativeInputCalls - previous.NativeInputCalls;
        var controlOperations = current.ControlOperations - previous.ControlOperations;
        var droppedAudioSamples = current.DroppedAudioSamples - previous.DroppedAudioSamples;

        if (droppedAudioSamples > 0 && CanReport(ref _lastAudioDropAlert, now))
        {
            _logger.Warning(
                $"Runtime issue: dropped_audio_samples={droppedAudioSamples} " +
                $"during {SampleInterval.TotalSeconds:F0}s.");
        }

        if (inputCalls >= MinimumSampleCount &&
            current.NativeInputAverageMilliseconds > SlowInputMilliseconds &&
            CanReport(ref _lastInputAlert, now))
        {
            _logger.Warning(
                $"Runtime issue: native_input_avg_ms=" +
                $"{current.NativeInputAverageMilliseconds:F3} " +
                $"over {inputCalls} calls.");
        }

        if (controlOperations >= MinimumSampleCount &&
            current.ControlAverageMilliseconds > SlowControlMilliseconds &&
            CanReport(ref _lastControlAlert, now))
        {
            _logger.Warning(
                $"Runtime issue: control_avg_ms=" +
                $"{current.ControlAverageMilliseconds:F3} " +
                $"over {controlOperations} operations.");
        }
    }

    private static bool CanReport(
        ref DateTimeOffset lastReport,
        DateTimeOffset now)
    {
        if (now - lastReport < AlertCooldown)
        {
            return false;
        }

        lastReport = now;
        return true;
    }

    public sealed record MetricsSample(
        long NativeInputCalls,
        double NativeInputAverageMilliseconds,
        long ControlOperations,
        double ControlAverageMilliseconds,
        long DroppedAudioSamples);
}

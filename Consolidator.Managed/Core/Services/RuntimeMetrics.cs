using System.Collections.Concurrent;
using System.Diagnostics;
using System.Text;

namespace Consolidator.Managed.Core.Services;

public sealed class RuntimeMetrics
{
    public static RuntimeMetrics Shared { get; } = new();

    private readonly long _startedAt = Stopwatch.GetTimestamp();
    private readonly ConcurrentDictionary<ulong, InstanceMetrics> _instances = new();
    private long _nativeInputCalls;
    private long _nativeInputTicks;
    private long _controlOperations;
    private long _controlOperationTicks;
    private long _registrySnapshots;
    private long _registryDeltas;
    private long _presentationActiveDeliveries;
    private long _presentationDiscarded;

    public void RecordNativeInput(long elapsedTicks)
    {
        Interlocked.Increment(ref _nativeInputCalls);
        Interlocked.Add(ref _nativeInputTicks, elapsedTicks);
    }

    public void RecordPresentationActiveDelivery() =>
        Interlocked.Increment(ref _presentationActiveDeliveries);

    public void RecordPresentationDiscarded() =>
        Interlocked.Increment(ref _presentationDiscarded);

    public void RecordControlOperation(long elapsedTicks)
    {
        Interlocked.Increment(ref _controlOperations);
        Interlocked.Add(ref _controlOperationTicks, elapsedTicks);
    }

    public void RecordRegistrySnapshot() =>
        Interlocked.Increment(ref _registrySnapshots);

    public void RecordRegistryDelta() =>
        Interlocked.Increment(ref _registryDeltas);

    public InstanceMetrics ForInstance(ulong instanceId) =>
        _instances.GetOrAdd(instanceId, static _ => new InstanceMetrics());

    public RuntimeMetricsMonitor.MetricsSample CaptureSample()
    {
        var nativeInputCalls = Interlocked.Read(ref _nativeInputCalls);
        var nativeInputTicks = Interlocked.Read(ref _nativeInputTicks);
        var operations = Interlocked.Read(ref _controlOperations);
        var operationTicks = Interlocked.Read(ref _controlOperationTicks);

        return new RuntimeMetricsMonitor.MetricsSample(
            nativeInputCalls,
            AverageMilliseconds(nativeInputCalls, nativeInputTicks),
            operations,
            AverageMilliseconds(operations, operationTicks),
            _instances.Values.Sum(instance => instance.DroppedAudioSamples));
    }

    public string FormatSnapshot()
    {
        var builder = new StringBuilder();
        var nativeInputCalls = Interlocked.Read(ref _nativeInputCalls);
        var nativeInputTicks = Interlocked.Read(ref _nativeInputTicks);
        var nativeInputMilliseconds = nativeInputCalls == 0
            ? 0
            : nativeInputTicks * 1000.0 / Stopwatch.Frequency / nativeInputCalls;
        var operations = Interlocked.Read(ref _controlOperations);
        var operationTicks = Interlocked.Read(ref _controlOperationTicks);
        var operationMilliseconds = operations == 0
            ? 0
            : operationTicks * 1000.0 / Stopwatch.Frequency / operations;
        var elapsedSeconds = Math.Max(
            (Stopwatch.GetTimestamp() - _startedAt) / (double)Stopwatch.Frequency,
            0.001);

        builder.Append("native_input_calls=").Append(nativeInputCalls)
            .Append(" native_input_avg_ms=").Append(nativeInputMilliseconds.ToString("F3"))
            .Append(" control_operations=").Append(operations)
            .Append(" control_avg_ms=").Append(operationMilliseconds.ToString("F3"))
            .Append(" registry_snapshots=").Append(Interlocked.Read(ref _registrySnapshots))
            .Append(" registry_deltas=").Append(Interlocked.Read(ref _registryDeltas))
            .Append(" native_control_frames=").Append(nativeInputCalls)
            .Append(" presentation_active_deliveries=").Append(Interlocked.Read(ref _presentationActiveDeliveries))
            .Append(" presentation_discarded=").Append(Interlocked.Read(ref _presentationDiscarded))
            .Append(" managed_allocated_bytes=").Append(GC.GetTotalAllocatedBytes(false));

        foreach (var entry in _instances.OrderBy(entry => entry.Key))
        {
            builder.Append(" instance=").Append(entry.Key)
                .Append(" fft=").Append(entry.Value.FftFrames)
                .Append(" fft_fps=").Append((entry.Value.FftFrames / elapsedSeconds).ToString("F2"))
                .Append(" dropped_audio_samples=").Append(entry.Value.DroppedAudioSamples);
        }

        return builder.ToString();
    }

    private static double AverageMilliseconds(long count, long ticks) =>
        count == 0
            ? 0
            : ticks * 1000.0 / Stopwatch.Frequency / count;

    public sealed class InstanceMetrics
    {
        private long _fftFrames;
        private long _droppedAudioSamples;

        public long FftFrames => Interlocked.Read(ref _fftFrames);
        public long DroppedAudioSamples => Interlocked.Read(ref _droppedAudioSamples);

        public void RecordFftFrame() => Interlocked.Increment(ref _fftFrames);
        public void RecordDroppedAudioSamples(long sampleCount) =>
            Interlocked.Add(ref _droppedAudioSamples, sampleCount);
    }
}

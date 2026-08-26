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
                .Append(" curves=").Append(entry.Value.CurveFrames)
                .Append(" curves_fps=").Append((entry.Value.CurveFrames / elapsedSeconds).ToString("F2"))
                .Append(" dropped_audio_samples=").Append(entry.Value.DroppedAudioSamples)
                .Append(" eq_avg_ms=").Append(entry.Value.EqualizerAverageMilliseconds.ToString("F3"));
        }

        return builder.ToString();
    }

    public sealed class InstanceMetrics
    {
        private long _fftFrames;
        private long _curveFrames;
        private long _droppedAudioSamples;
        private long _equalizerCalculations;
        private long _equalizerCalculationTicks;

        public long FftFrames => Interlocked.Read(ref _fftFrames);
        public long CurveFrames => Interlocked.Read(ref _curveFrames);
        public long DroppedAudioSamples => Interlocked.Read(ref _droppedAudioSamples);

        public double EqualizerAverageMilliseconds
        {
            get
            {
                var calculations = Interlocked.Read(ref _equalizerCalculations);
                var ticks = Interlocked.Read(ref _equalizerCalculationTicks);
                return calculations == 0
                    ? 0
                    : ticks * 1000.0 / Stopwatch.Frequency / calculations;
            }
        }

        public void RecordFftFrame() => Interlocked.Increment(ref _fftFrames);
        public void RecordCurveFrame() => Interlocked.Increment(ref _curveFrames);
        public void RecordDroppedAudioSamples(long sampleCount) =>
            Interlocked.Add(ref _droppedAudioSamples, sampleCount);

        public void RecordEqualizerCalculation(long elapsedTicks)
        {
            Interlocked.Increment(ref _equalizerCalculations);
            Interlocked.Add(ref _equalizerCalculationTicks, elapsedTicks);
        }
    }
}

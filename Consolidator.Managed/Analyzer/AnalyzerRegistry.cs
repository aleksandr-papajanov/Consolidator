using System.Collections.Concurrent;
using System.Diagnostics;
using System.Numerics;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.Dsp;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Analyzer;

public sealed class AnalyzerRegistry
{
    private const int CurvePointCount = 256;
    private readonly Dictionary<InstanceId, AnalyzerInstance> _instances = new();
    private readonly HashSet<(InstanceId InstanceId, int BankIndex, CurveKind Kind)> _pendingBanks = new();
    private readonly ConcurrentDictionary<InstanceId, AudioCapture> _captures = new();
    private readonly ConcurrentDictionary<(InstanceId InstanceId, int BankIndex, CurveKind Kind), DirtyBank> _dirtyBanks = new();
    private long _revision;
    private readonly object _lock = new();

    public event Action<InstanceId, int, CurveKind>? CurveChanged;

    public enum CurveKind
    {
        Equalizer,
        CompressorDetector,
        SaturatorDetector
    }

    public void Register(InstanceId instanceId, DspState state)
    {
        ArgumentNullException.ThrowIfNull(state);

        lock (_lock)
        {
            var instance = new AnalyzerInstance(state);
            _instances.Add(instanceId, instance);
        }
    }

    public void Unregister(InstanceId instanceId)
    {
        lock (_lock)
        {
            _instances.Remove(instanceId);
            _pendingBanks.RemoveWhere(key => key.InstanceId == instanceId);
        }

        foreach (var dirtyBank in _dirtyBanks.Keys
            .Where(key => key.InstanceId == instanceId)
            .ToArray())
        {
            _dirtyBanks.TryRemove(dirtyBank, out _);
        }

        _captures.TryRemove(instanceId, out _);
    }

    public void RemoveCapture(InstanceId instanceId)
    {
        _captures.TryRemove(instanceId, out _);
    }

    public void PrepareCapture(
        InstanceId instanceId,
        int blockSize,
        int queueLength)
    {
        _captures.AddOrUpdate(
            instanceId,
            _ => new AudioCapture(blockSize, queueLength),
            (_, capture) =>
            {
                capture.Resize(blockSize, queueLength);
                return capture;
            });
    }

    public bool TryGetCapture(
        InstanceId instanceId,
        out AudioCapture capture)
    {
        return _captures.TryGetValue(instanceId, out capture!);
    }

    public void SetSampleRate(InstanceId instanceId, double sampleRate)
    {
        if (sampleRate <= 0 || double.IsNaN(sampleRate) || double.IsInfinity(sampleRate))
        {
            return;
        }

        lock (_lock)
        {
            if (_instances.TryGetValue(instanceId, out var instance))
            {
                instance.SetSampleRate(sampleRate);
            }
        }
    }

    public void UpdateFilter(InstanceId instanceId, StatePath path)
    {
        ArgumentNullException.ThrowIfNull(path);

        if (!TryGetFilterAddress(path, out var bankIndex, out _, out var kind))
        {
            return;
        }

        lock (_lock)
        {
            if (_instances.TryGetValue(instanceId, out var instance))
            {
                instance.Invalidate(kind, bankIndex);
                _pendingBanks.Add((instanceId, bankIndex, kind));
            }
        }
    }

    public void CapturePendingInputs()
    {
        lock (_lock)
        {
            foreach (var key in _pendingBanks)
            {
                if (!_instances.TryGetValue(key.InstanceId, out var instance))
                {
                    continue;
                }

                instance.Capture(key.Kind, key.BankIndex);
                _dirtyBanks[key] = new DirtyBank(
                    Interlocked.Increment(ref _revision));
            }
            _pendingBanks.Clear();
        }
    }

    public void ProcessDirtyBanks(
        int maximumCount,
        Func<InstanceId, int, CurveKind, bool> hasRecipient)
    {
        if (maximumCount <= 0)
        {
            throw new ArgumentOutOfRangeException(nameof(maximumCount));
        }
        ArgumentNullException.ThrowIfNull(hasRecipient);

        var processedCount = 0;
        var dirtyBanks = _dirtyBanks.ToArray();
        Array.Sort(
            dirtyBanks,
            (left, right) => left.Value.Revision.CompareTo(
                right.Value.Revision));
        foreach (var entry in dirtyBanks)
        {
            var shouldPublish = hasRecipient(
                entry.Key.InstanceId,
                entry.Key.BankIndex,
                entry.Key.Kind);
            if (shouldPublish && processedCount >= maximumCount)
            {
                continue;
            }
            if (!_dirtyBanks.TryRemove(entry.Key, out var dirtyBank))
            {
                continue;
            }
            if (!shouldPublish)
            {
                continue;
            }
            processedCount++;

            if (_dirtyBanks.TryGetValue(entry.Key, out var newer) &&
                newer.Revision > dirtyBank.Revision)
            {
                continue;
            }

            CurveChanged?.Invoke(
                entry.Key.InstanceId,
                entry.Key.BankIndex,
                entry.Key.Kind);
        }
    }

    public bool TryGetCurves(
        InstanceId instanceId,
        int bankIndex,
        CurveKind kind,
        out EqualizerCurves curves)
    {
        if (kind != CurveKind.Equalizer)
        {
            bankIndex = 0;
        }

        var startedAt = Stopwatch.GetTimestamp();
        lock (_lock)
        {
            if (_pendingBanks.Contains((instanceId, bankIndex, kind)))
            {
                curves = null!;
                return false;
            }

            if (_instances.TryGetValue(instanceId, out var instance) &&
                instance.TryGetCurves(
                    kind,
                    bankIndex,
                    out curves!,
                    out var recalculated))
            {
                if (recalculated)
                {
                    RuntimeMetrics.Shared.ForInstance(instanceId.Value)
                        .RecordEqualizerCalculation(
                            Stopwatch.GetTimestamp() - startedAt);
                }
                return true;
            }
        }

        curves = null!;
        return false;
    }

    private static bool TryGetFilterAddress(
        StatePath path,
        out int bankIndex,
        out int filterIndex,
        out CurveKind kind)
    {
        bankIndex = 0;
        filterIndex = 0;
        kind = CurveKind.Equalizer;
        var nodes = path.Nodes;
        for (var index = 0; index < nodes.Count - 1; index++)
        {
            if (nodes[index] != StateNodeIds.EqualizerBank ||
                nodes[index + 1].Value < 100 ||
                nodes[index + 1].Value >= 100 + DspConstants.BankCount)
            {
                continue;
            }

            bankIndex = (int)nodes[index + 1].Value - 100;
            if (index + 2 == nodes.Count - 1 &&
                nodes[index + 2] == StateNodeIds.Bypass)
            {
                return true;
            }

            if (index + 3 >= nodes.Count ||
                nodes[index + 2] != StateNodeIds.Filter ||
                nodes[index + 3].Value < 200 ||
                nodes[index + 3].Value >= 200 + DspConstants.EqualizerFilterCount)
            {
                return false;
            }

            filterIndex = (int)nodes[index + 3].Value - 200;
            return nodes[^1] is
                { } leaf && (leaf == StateNodeIds.Frequency ||
                leaf == StateNodeIds.Q ||
                leaf == StateNodeIds.Gain ||
                leaf == StateNodeIds.Bypass);
        }

        for (var index = 0; index + 5 < nodes.Count; index++)
        {
            if (nodes[index] != StateNodeIds.Dsp ||
                (nodes[index + 1] != StateNodeIds.Compressor &&
                    nodes[index + 1] != StateNodeIds.Saturator) ||
                nodes[index + 2] != StateNodeIds.Detector ||
                nodes[index + 3] != StateNodeIds.Filter ||
                nodes[index + 4].Value < 200 ||
                nodes[index + 4].Value >= 200 + DspConstants.DetectorFilterCount)
            {
                continue;
            }

            filterIndex = (int)nodes[index + 4].Value - 200;
            kind = nodes[index + 1] == StateNodeIds.Compressor
                ? CurveKind.CompressorDetector
                : CurveKind.SaturatorDetector;
            return nodes[^1] is
                { } leaf && (leaf == StateNodeIds.Frequency ||
                leaf == StateNodeIds.Q || leaf == StateNodeIds.Gain ||
                leaf == StateNodeIds.Bypass);
        }

        return false;
    }

    private sealed record DirtyBank(long Revision);

    private readonly record struct FilterInput(
        float Frequency,
        float Q,
        float Gain,
        bool Bypass);

    private readonly record struct BankInput(
        bool Bypass,
        FilterInput[] Filters);

    private sealed class AnalyzerInstance
    {
        private static readonly float[] UnityCurve =
            Enumerable.Repeat(0.5F, CurvePointCount).ToArray();
        private readonly DspState _state;
        private readonly BankInput[] _bankInputs;
        private BankInput _compressorDetectorInput;
        private BankInput _saturatorDetectorInput;
        private readonly EqualizerBankCache?[] _banks;
        private readonly EqualizerBankCache?[] _compressorDetectors;
        private readonly EqualizerBankCache?[] _saturatorDetectors;
        private readonly object _cacheLock = new();
        private float[]? _allBanksCurve;
        private double _sampleRate = 48000.0;

        public AnalyzerInstance(DspState state)
        {
            _state = state;
            _bankInputs = state.EqualizerBanks
                .Select(Capture)
                .ToArray();
            _compressorDetectorInput = Capture(
                state.Compressor.Detector.Filters);
            _saturatorDetectorInput = Capture(
                state.Saturator.Detector.Filters);
            _banks = new EqualizerBankCache?[state.EqualizerBanks.Length];
            _compressorDetectors = new EqualizerBankCache?[1];
            _saturatorDetectors = new EqualizerBankCache?[1];
        }

        public BankInput Capture(CurveKind kind, int bankIndex)
        {
            var input = kind switch
            {
                CurveKind.Equalizer => Capture(
                    _state.EqualizerBanks[bankIndex]),
                CurveKind.CompressorDetector => Capture(
                    _state.Compressor.Detector.Filters),
                CurveKind.SaturatorDetector => Capture(
                    _state.Saturator.Detector.Filters),
                _ => throw new ArgumentOutOfRangeException(nameof(kind))
            };
            SetInput(kind, bankIndex, input);
            return input;
        }

        public void Invalidate(CurveKind kind, int bankIndex)
        {
            lock (_cacheLock)
            {
                SetCache(kind, bankIndex, null);
                if (kind is CurveKind.Equalizer)
                {
                    _allBanksCurve = null;
                }
            }
        }

        public void SetSampleRate(double sampleRate)
        {
            lock (_cacheLock)
            {
                if (_sampleRate == sampleRate)
                {
                    return;
                }

                _sampleRate = sampleRate;
                for (var bankIndex = 0;
                    bankIndex < _banks.Length;
                    bankIndex++)
                {
                    _banks[bankIndex]?.Rebuild(
                        _bankInputs[bankIndex],
                        _sampleRate);
                }
                if (_compressorDetectors[0] is { } compressorDetector)
                {
                    compressorDetector.Rebuild(
                        _compressorDetectorInput,
                        _sampleRate);
                }
                if (_saturatorDetectors[0] is { } saturatorDetector)
                {
                    saturatorDetector.Rebuild(
                        _saturatorDetectorInput,
                        _sampleRate);
                }
                _allBanksCurve = null;
            }
        }

        public bool TryGetCurves(
            CurveKind kind,
            int bankIndex,
            out EqualizerCurves curves,
            out bool recalculated)
        {
            lock (_cacheLock)
            {
                var bank = GetCache(kind, bankIndex);
                recalculated = bank is null ||
                    kind is CurveKind.Equalizer && _allBanksCurve is null;
                if (bank is null)
                {
                    var input = GetInput(kind, bankIndex);
                    bank = CreateCache(input.Filters.Length);
                    bank.Rebuild(input, _sampleRate);
                    SetCache(kind, bankIndex, bank);
                }
                curves = new EqualizerCurves(
                    bank.IsActive,
                    bank.EqualizerCurve,
                    bank.FilterCurves,
                    kind == CurveKind.Equalizer
                        ? GetAllBanksCurve()
                        : UnityCurve);
                return true;
            }
        }

        private static EqualizerBankCache CreateCache(int filterCount)
        {
            return new EqualizerBankCache(filterCount);
        }

        private EqualizerBankCache? GetCache(CurveKind kind, int bankIndex)
        {
            return kind switch
            {
                CurveKind.Equalizer => _banks[bankIndex],
                CurveKind.CompressorDetector => _compressorDetectors[bankIndex],
                CurveKind.SaturatorDetector => _saturatorDetectors[bankIndex],
                _ => throw new ArgumentOutOfRangeException(nameof(kind))
            };
        }

        private void SetCache(
            CurveKind kind,
            int bankIndex,
            EqualizerBankCache? cache)
        {
            switch (kind)
            {
                case CurveKind.Equalizer:
                    _banks[bankIndex] = cache;
                    break;
                case CurveKind.CompressorDetector:
                    _compressorDetectors[bankIndex] = cache;
                    break;
                case CurveKind.SaturatorDetector:
                    _saturatorDetectors[bankIndex] = cache;
                    break;
                default:
                    throw new ArgumentOutOfRangeException(nameof(kind));
            }
        }

        private float[] GetAllBanksCurve()
        {
            return _allBanksCurve ??= BuildAllBanksCurve();
        }

        private float[] BuildAllBanksCurve()
        {
            for (var bankIndex = 0; bankIndex < _banks.Length; bankIndex++)
            {
                if (_banks[bankIndex] is null)
                {
                    var bank = CreateCache(_bankInputs[bankIndex].Filters.Length);
                    bank.Rebuild(_bankInputs[bankIndex], _sampleRate);
                    _banks[bankIndex] = bank;
                }
            }

            var curve = new float[CurvePointCount];
            for (var point = 0; point < curve.Length; point++)
            {
                var normalized = point / (double)(curve.Length - 1);
                var frequency = 20.0 * Math.Pow(1000.0, normalized);
                var decibels = 0.0;
                foreach (var bank in _banks)
                {
                    if (bank is not null && bank.IsActive && bank.HasResponse)
                    {
                        decibels += bank.DecibelsAt(frequency, _sampleRate);
                    }
                }

                curve[point] = EqualizerBankCache.ToNormalizedDecibels(decibels);
            }

            return curve;
        }

        private static BankInput Capture(EqualizerBankState bank)
        {
            return Capture(bank.Filters, bank.Bypass.Value);
        }

        private static BankInput Capture(
            IReadOnlyList<FilterState> filters,
            bool bypass = false)
        {
            return new BankInput(
                bypass,
                filters.Select(filter => new FilterInput(
                    filter.FrequencyHz.Value,
                    filter.Q.Value,
                    filter.GainDb.Value,
                    filter.Bypass.Value)).ToArray());
        }

        private BankInput GetInput(CurveKind kind, int bankIndex)
        {
            return kind switch
            {
                CurveKind.Equalizer => _bankInputs[bankIndex],
                CurveKind.CompressorDetector => _compressorDetectorInput,
                CurveKind.SaturatorDetector => _saturatorDetectorInput,
                _ => throw new ArgumentOutOfRangeException(nameof(kind))
            };
        }

        private void SetInput(CurveKind kind, int bankIndex, BankInput input)
        {
            switch (kind)
            {
                case CurveKind.Equalizer:
                    _bankInputs[bankIndex] = input;
                    break;
                case CurveKind.CompressorDetector:
                    _compressorDetectorInput = input;
                    break;
                case CurveKind.SaturatorDetector:
                    _saturatorDetectorInput = input;
                    break;
                default:
                    throw new ArgumentOutOfRangeException(nameof(kind));
            }
        }
    }

    public sealed class EqualizerCurves
    {
        public EqualizerCurves(
            bool active,
            IReadOnlyList<float> combined,
            IReadOnlyList<EqualizerFilterCurve> filters,
            IReadOnlyList<float> allBanks)
        {
            Active = active;
            Combined = combined;
            Filters = filters;
            AllBanks = allBanks;
        }

        public bool Active { get; }
        public IReadOnlyList<float> Combined { get; }
        public IReadOnlyList<EqualizerFilterCurve> Filters { get; }
        public IReadOnlyList<float> AllBanks { get; }
    }

    public sealed class EqualizerFilterCurve
    {
        public EqualizerFilterCurve(bool active, IReadOnlyList<float> values)
        {
            Active = active;
            Values = values;
        }

        public bool Active { get; }
        public IReadOnlyList<float> Values { get; }
    }

    private sealed class EqualizerBankCache
    {
        private readonly FilterCache[] _filters;
        private float[]? _equalizerCurve;
        private EqualizerFilterCurve[]? _filterCurves;
        private bool _isActive;

        public EqualizerBankCache(int filterCount)
        {
            _filters = Enumerable.Range(0, filterCount)
                .Select(_ => new FilterCache(BiquadType.Bell))
                .ToArray();
            _isActive = true;
        }

        public IReadOnlyList<float> EqualizerCurve =>
            _equalizerCurve ?? Array.Empty<float>();

        public IReadOnlyList<EqualizerFilterCurve> FilterCurves =>
            _filterCurves ?? Array.Empty<EqualizerFilterCurve>();

        public bool IsActive => _isActive;

        public bool HasResponse => _filters.Any(filter =>
            filter.Enabled && Math.Abs(filter.GainDb) > 1e-6);

        public void Rebuild(BankInput input, double sampleRate)
        {
            _isActive = !input.Bypass;
            for (var filterIndex = 0; filterIndex < _filters.Length; filterIndex++)
            {
                var filter = input.Filters[filterIndex];
                _filters[filterIndex].Recalculate(
                    filter.Frequency,
                    filter.Q,
                    filter.Gain,
                    filter.Bypass,
                    sampleRate);
            }
            _filterCurves = BuildFilterCurves(sampleRate);
            _equalizerCurve = BuildCurve(sampleRate);
        }

        public double DecibelsAt(double frequencyHz, double sampleRate)
        {
            if (!IsActive)
            {
                return 0.0;
            }

            var decibels = 0.0;
            foreach (var filter in _filters)
            {
                if (filter.Enabled)
                {
                    decibels += filter.DecibelsAt(frequencyHz, sampleRate);
                }
            }

            return decibels;
        }

        public static float ToNormalizedDecibels(double decibels)
        {
            return (float)Math.Clamp(1.0 - (decibels + 24.0) / 48.0, 0.0, 1.0);
        }

        private EqualizerFilterCurve[] BuildFilterCurves(double sampleRate)
        {
            return _filters
                .Select(filter => new EqualizerFilterCurve(
                    filter.Enabled,
                    BuildCurve(sampleRate, filter)))
                .ToArray();
        }

        private float[] BuildCurve(double sampleRate)
        {
            var curve = new float[CurvePointCount];
            for (var point = 0; point < curve.Length; point++)
            {
                var normalized = point / (double)(curve.Length - 1);
                var frequency = 20.0 * Math.Pow(1000.0, normalized);
                curve[point] = ToNormalizedDecibels(DecibelsAt(frequency, sampleRate));
            }

            return curve;
        }

        private static float[] BuildCurve(double sampleRate, FilterCache filter)
        {
            var curve = new float[CurvePointCount];
            for (var point = 0; point < curve.Length; point++)
            {
                var normalized = point / (double)(curve.Length - 1);
                var frequency = 20.0 * Math.Pow(1000.0, normalized);
                var decibels = filter.Enabled
                    ? filter.DecibelsAt(frequency, sampleRate)
                    : 0.0;
                curve[point] = ToNormalizedDecibels(decibels);
            }

            return curve;
        }
    }

    private sealed class FilterCache
    {
        public FilterCache(BiquadType type)
        {
            Type = type;
        }

        public BiquadType Type { get; }
        public BiquadCoefficients Coefficients { get; private set; }
        public bool Enabled { get; private set; }
        public double GainDb { get; private set; }

        public void Recalculate(
            float frequency,
            float q,
            float gain,
            bool bypass,
            double sampleRate)
        {
            Coefficients = BiquadCalculator.Calculate(
                Type,
                frequency,
                q,
                gain,
                sampleRate);
            Enabled = !bypass;
            GainDb = gain;
        }

        public double MagnitudeAt(double frequencyHz, double sampleRate)
        {
            var omega = 2.0 * Math.PI * frequencyHz / sampleRate;
            var z = Complex.Exp(new Complex(0.0, -omega));
            var numerator = Coefficients.B0 +
                Coefficients.B1 * z +
                Coefficients.B2 * z * z;
            var denominator = 1.0 + Coefficients.A1 * z + Coefficients.A2 * z * z;
            return (numerator / denominator).Magnitude;
        }

        public double DecibelsAt(double frequencyHz, double sampleRate)
        {
            return 20.0 * Math.Log10(Math.Max(MagnitudeAt(frequencyHz, sampleRate), 1e-12));
        }
    }

    public sealed class AudioCapture
    {
        private const int ChannelCount = 4;
        private const int FftSize = 1024;
        private const int SpectrumBinCount = FftSize / 2 + 1;
        private readonly double[][] _channels;
        private readonly int _capacity;
        private int _writeIndex;
        private int _readIndex;
        private long _droppedSamples;

        public Complex[] MainFft { get; } = new Complex[FftSize];
        public Complex[] ReferenceFft { get; } = new Complex[FftSize];
        public float[] MainSpectrum { get; } = new float[SpectrumBinCount];
        public float[] ReferenceSpectrum { get; } = new float[SpectrumBinCount];

        public AudioCapture(int blockSize, int queueLength)
        {
            _capacity = Math.Max(2048, blockSize * Math.Max(2, queueLength));
            _channels = Enumerable.Range(0, ChannelCount)
                .Select(_ => new double[_capacity])
                .ToArray();
        }

        public long DroppedSamples => Interlocked.Read(ref _droppedSamples);

        public void Resize(int blockSize, int queueLength)
        {
            // Capture dimensions are fixed at activation; resizing is intentionally a no-op.
        }

        public unsafe int Enqueue(
            double* mainLeft,
            double* mainRight,
            double* referenceLeft,
            double* referenceRight,
            nuint frameCount)
        {
            var count = (int)Math.Min((nuint)int.MaxValue, frameCount);
            var writeIndex = _writeIndex;
            var readIndex = Volatile.Read(ref _readIndex);
            var available = _capacity - (writeIndex - readIndex);
            var writable = Math.Min(count, available);
            for (var index = 0; index < writable; index++)
            {
                var ringIndex = (writeIndex + index) % _capacity;
                _channels[0][ringIndex] = mainLeft[index];
                _channels[1][ringIndex] = mainRight[index];
                _channels[2][ringIndex] = referenceLeft is null ? 0 : referenceLeft[index];
                _channels[3][ringIndex] = referenceRight is null ? 0 : referenceRight[index];
            }

            if (writable > 0)
            {
                Volatile.Write(ref _writeIndex, writeIndex + writable);
            }

            var dropped = count - writable;
            if (dropped > 0)
            {
                Interlocked.Add(ref _droppedSamples, dropped);
            }

            return dropped;
        }

        public bool TryReadWindow(
            Complex[] mainFft,
            Complex[] referenceFft,
            IReadOnlyList<double> window,
            int windowSize,
            int hopSize)
        {
            var readIndex = _readIndex;
            var writeIndex = Volatile.Read(ref _writeIndex);
            if (writeIndex - readIndex < windowSize)
            {
                return false;
            }

            if (writeIndex - readIndex > windowSize)
            {
                readIndex = writeIndex - windowSize;
            }

            for (var index = 0; index < windowSize; index++)
            {
                var ringIndex = (readIndex + index) % _capacity;
                var multiplier = window[index];
                mainFft[index] = new Complex(
                    (_channels[0][ringIndex] + _channels[1][ringIndex]) * 0.5 * multiplier,
                    0);
                referenceFft[index] = new Complex(
                    (_channels[2][ringIndex] + _channels[3][ringIndex]) * 0.5 * multiplier,
                    0);
            }

            Volatile.Write(ref _readIndex, readIndex + hopSize);
            return true;
        }
    }
}

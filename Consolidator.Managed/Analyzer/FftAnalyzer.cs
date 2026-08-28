using System.Collections.Concurrent;
using System.Diagnostics;
using System.Numerics;
using MathNet.Numerics.IntegralTransforms;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.Protocol.Messages;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Analyzer;

public sealed class FftAnalyzer : IInstanceAudioInputService, IInstancePreparationHandler, IDisposable
{
    private const int FftSize = 1024;
    private const int HopSize = FftSize / 2;
    private const int QueueLength = 4;
    private const int SpectrumBinCount = FftSize / 2 + 1;
    private const int SpectrumIntervalMilliseconds = 33;
    private const double MinimumDecibels = -90.0;
    private const double DefaultSampleRate = 48000.0;

    private readonly ConcurrentDictionary<InstanceId, AudioCapture> _captures = new();
    private readonly ConcurrentDictionary<InstanceId, BankAddress?> _focusedBanks = new();
    private readonly ConcurrentDictionary<InstanceId, Preparation> _preparations = new();
    private ulong _activeViewerId;
    private ulong _activeSourceId;
    private readonly StateTopologyObserver _topologyObserver;
    private readonly IPresentationTransport _transport;
    private readonly CancellationTokenSource _cancellation = new();
    private readonly Task _worker;
    private readonly double[] _window = CreateWindow();
    private int _disposeStarted;

    internal FftAnalyzer(
        StateTopologyObserver topology,
        IPresentationTransport transport)
    {
        ArgumentNullException.ThrowIfNull(topology);
        ArgumentNullException.ThrowIfNull(transport);

        _transport = transport;
        _topologyObserver = topology;
        topology.FocusedBankChangedEvent += FocusedBankChanged;
        _worker = Task.Run(ProcessCapturesAsync);
    }

    public void Prepare(
        InstanceId instanceId,
        double sampleRate,
        nuint maximumFrameCount)
    {
        if (!double.IsFinite(sampleRate) || sampleRate <= 0)
        {
            return;
        }

        var preparation = new Preparation(sampleRate, maximumFrameCount);
        if (_preparations.TryGetValue(instanceId, out var previous) &&
            previous == preparation)
        {
            return;
        }
        _preparations[instanceId] = preparation;

        if (maximumFrameCount != 0 &&
            IsSourceDemanded(instanceId))
        {
            var blockSize = (int)Math.Min(maximumFrameCount, 8192);
            PrepareCapture(instanceId, blockSize);
        }

        var focusedBank = GetFocusedBank(Volatile.Read(ref _activeViewerId));
        if (focusedBank is { } bank && bank.InstanceId == instanceId)
        {
            PublishConfiguration(instanceId);
        }
    }

    internal void RemoveInstance(InstanceId instanceId)
    {
        _captures.TryRemove(instanceId, out _);
        _preparations.TryRemove(instanceId, out _);
    }

    internal void PublishEqualizerState(ManagedState state)
    {
        ArgumentNullException.ThrowIfNull(state);

        var sourceInstanceId = state.Instance.InstanceId;
        var targetId = GetSpectrumRecipient(sourceInstanceId);
        if (targetId == 0)
        {
            return;
        }

        var banks = state.Dsp.EqualizerBanks;
        var filterCount = banks.Length == 0 ? 0 : banks[0].Filters.Length;
        var atoms = new List<Atom>(5 + banks.Length * (1 + filterCount * 4))
        {
            new(AtomType.Integer, 1, 0, null),
            new(AtomType.Integer, (long)sourceInstanceId.Value, 0, null),
            new(AtomType.Integer, banks.Length, 0, null),
            new(AtomType.Integer, filterCount, 0, null),
            new(
                AtomType.Integer,
                state.Dsp.Equalizer.Bypass.Value ? 0 : 1,
                0,
                null)
        };
        foreach (var bank in banks)
        {
            atoms.Add(new Atom(
                AtomType.Integer,
                bank.Bypass.Value ? 0 : 1,
                0,
                null));
            foreach (var filter in bank.Filters)
            {
                atoms.Add(new Atom(
                    AtomType.Integer,
                    filter.Bypass.Value ? 0 : 1,
                    0,
                    null));
                atoms.Add(new Atom(
                    AtomType.Float,
                    0,
                    filter.FrequencyHz.Value,
                    null));
                atoms.Add(new Atom(AtomType.Float, 0, filter.Q.Value, null));
                atoms.Add(new Atom(
                    AtomType.Float,
                    0,
                    filter.GainDb.Value,
                    null));
            }
        }

        _transport.Send(new ProtocolOutput(
            [targetId],
            "analyzer_equalizer_state",
            atoms,
            DeliverySemantics.ActivePresentation));
    }

    public void SetInstanceActive(InstanceId instanceId, bool active)
    {
        if (active)
        {
            var previousViewerId = Interlocked.Exchange(
                ref _activeViewerId,
                instanceId.Value);
            if (previousViewerId == instanceId.Value)
            {
                return;
            }

            var previousBank = GetFocusedBank(previousViewerId);
            var focusedBank = GetFocusedBank(instanceId.Value);
            Volatile.Write(
                ref _activeSourceId,
                focusedBank?.InstanceId.Value ?? 0);
            StopCapture(previousBank, focusedBank);
            if (focusedBank is { } bank)
            {
                PrepareCapture(bank.InstanceId);
                PublishConfiguration(bank.InstanceId);
            }
            return;
        }

        if (Interlocked.CompareExchange(
            ref _activeViewerId,
            0,
            instanceId.Value) != instanceId.Value)
        {
            return;
        }

        Volatile.Write(ref _activeSourceId, 0);
        StopCapture(GetFocusedBank(instanceId.Value), null);
    }

    public unsafe void ReceiveAudio(
        InstanceId instanceId,
        double* mainLeft,
        double* mainRight,
        double* referenceLeft,
        double* referenceRight,
        nuint frameCount)
    {
        if (mainLeft is null || mainRight is null || frameCount == 0 ||
            !IsSourceDemanded(instanceId) ||
            !_captures.TryGetValue(instanceId, out var capture))
        {
            return;
        }

        var droppedSamples = capture.Enqueue(
            mainLeft,
            mainRight,
            referenceLeft,
            referenceRight,
            frameCount);
        if (droppedSamples > 0)
        {
            RuntimeMetrics.Shared.ForInstance(instanceId.Value)
                .RecordDroppedAudioSamples(droppedSamples);
        }
    }

    public void Dispose()
    {
        if (Interlocked.Exchange(ref _disposeStarted, 1) != 0)
        {
            return;
        }

        _topologyObserver.FocusedBankChangedEvent -= FocusedBankChanged;
        _cancellation.Cancel();
        try
        {
            _worker.GetAwaiter().GetResult();
        }
        catch (OperationCanceledException)
        {
        }

        _cancellation.Dispose();
    }

    private void FocusedBankChanged(InstanceId instanceId, BankAddress? focusedBank)
    {
        _focusedBanks.TryGetValue(instanceId, out var previousFocusedBank);
        if (focusedBank is null)
        {
            _focusedBanks.TryRemove(instanceId, out _);
            if (Interlocked.CompareExchange(
                ref _activeViewerId,
                0,
                instanceId.Value) == instanceId.Value)
            {
                Volatile.Write(ref _activeSourceId, 0);
                StopCapture(previousFocusedBank, null);
            }
            return;
        }

        _focusedBanks[instanceId] = focusedBank;
        if (IsViewerActive(instanceId))
        {
            Volatile.Write(
                ref _activeSourceId,
                focusedBank.Value.InstanceId.Value);
            StopCapture(previousFocusedBank, focusedBank);
            PrepareCapture(focusedBank.Value.InstanceId);
            PublishConfiguration(focusedBank.Value.InstanceId);
        }
    }

    private async Task ProcessCapturesAsync()
    {
        var lastSpectrumProcessing = 0L;
        while (!_cancellation.IsCancellationRequested)
        {
            var timestamp = Stopwatch.GetTimestamp();
            if (lastSpectrumProcessing == 0 ||
                Stopwatch.GetElapsedTime(lastSpectrumProcessing, timestamp) >=
                    TimeSpan.FromMilliseconds(SpectrumIntervalMilliseconds))
            {
                ProcessNextCapture();
                lastSpectrumProcessing = timestamp;
            }

            await Task.Delay(15, _cancellation.Token);
        }
    }

    private void ProcessNextCapture()
    {
        var sourceId = Volatile.Read(ref _activeSourceId);
        if (sourceId == 0)
        {
            return;
        }

        var sourceInstanceId = new InstanceId(sourceId);
        if (!_captures.TryGetValue(sourceInstanceId, out var capture))
        {
            return;
        }

        if (capture.TryReadWindow(
            capture.MainFft,
            capture.ReferenceFft,
            _window,
            FftSize,
            HopSize))
        {
            AnalyzeWindow(sourceInstanceId, capture);
        }
    }

    private void AnalyzeWindow(
        InstanceId sourceInstanceId,
        AudioCapture capture)
    {
        Fourier.Forward(capture.MainFft, FourierOptions.Matlab);
        Fourier.Forward(capture.ReferenceFft, FourierOptions.Matlab);
        FillSpectrum(capture.MainFft, capture.MainSpectrum);
        FillSpectrum(capture.ReferenceFft, capture.ReferenceSpectrum);
        Publish(sourceInstanceId, capture.MainSpectrum, capture.ReferenceSpectrum);
    }

    private static double[] CreateWindow()
    {
        var window = new double[FftSize];
        for (var index = 0; index < window.Length; index++)
        {
            window[index] = 0.5 - 0.5 * Math.Cos(
                2.0 * Math.PI * index / (window.Length - 1));
        }

        return window;
    }

    private readonly record struct Preparation(
        double SampleRate,
        nuint MaximumFrameCount);

    private static void FillSpectrum(Complex[] fft, float[] spectrum)
    {
        for (var index = 0; index < spectrum.Length; index++)
        {
            var magnitude = fft[index].Magnitude / FftSize;
            var decibels = 20.0 * Math.Log10(Math.Max(magnitude, 1e-12));
            spectrum[index] = (float)Math.Clamp(
                1.0 - ((decibels - MinimumDecibels) / -MinimumDecibels),
                0.0,
                1.0);
        }
    }

    private void Publish(
        InstanceId sourceInstanceId,
        float[] mainSpectrum,
        float[] referenceSpectrum)
    {
        var targetId = GetSpectrumRecipient(sourceInstanceId);
        if (targetId == 0)
        {
            return;
        }

        var atoms = new List<Atom>(3 + SpectrumBinCount * 2)
        {
            new(AtomType.Integer, 1, 0, null),
            new(AtomType.Integer, (long)sourceInstanceId.Value, 0, null),
            new(AtomType.Integer, FftSize, 0, null)
        };
        for (var index = 0; index < mainSpectrum.Length; index++)
        {
            atoms.Add(new Atom(AtomType.Float, 0, mainSpectrum[index], null));
        }
        for (var index = 0; index < referenceSpectrum.Length; index++)
        {
            atoms.Add(new Atom(AtomType.Float, 0, referenceSpectrum[index], null));
        }

        _transport.Send(new ProtocolOutput(
            [targetId],
            "fft",
            atoms,
            DeliverySemantics.LatestAnalysis));
        RuntimeMetrics.Shared.ForInstance(sourceInstanceId.Value)
            .RecordFftFrame();
    }

    private bool IsViewerActive(InstanceId instanceId)
    {
        return Volatile.Read(ref _activeViewerId) == instanceId.Value;
    }

    private bool IsSourceDemanded(InstanceId instanceId)
    {
        return Volatile.Read(ref _activeSourceId) == instanceId.Value;
    }

    private BankAddress? GetFocusedBank(ulong viewerId)
    {
        return viewerId != 0 && _focusedBanks.TryGetValue(
            new InstanceId(viewerId),
            out var focusedBank)
                ? focusedBank
                : null;
    }

    private void PrepareCapture(InstanceId sourceInstanceId)
    {
        var blockSize = _preparations.TryGetValue(
            sourceInstanceId,
            out var preparation) && preparation.MaximumFrameCount != 0
                ? (int)Math.Min(preparation.MaximumFrameCount, 8192)
                : FftSize;
        PrepareCapture(sourceInstanceId, blockSize);
    }

    private void PrepareCapture(InstanceId sourceInstanceId, int blockSize)
    {
        _captures.GetOrAdd(
            sourceInstanceId,
            _ => new AudioCapture(blockSize, QueueLength));
    }

    private void StopCapture(BankAddress? previous, BankAddress? current)
    {
        if (previous is { } previousBank &&
            previousBank.InstanceId != current?.InstanceId)
        {
            _captures.TryRemove(previousBank.InstanceId, out _);
        }
    }

    private void PublishConfiguration(InstanceId sourceInstanceId)
    {
        var targetId = GetSpectrumRecipient(sourceInstanceId);
        if (targetId == 0)
        {
            return;
        }

        var sampleRate = _preparations.TryGetValue(
            sourceInstanceId,
            out var preparation)
                ? preparation.SampleRate
                : DefaultSampleRate;
        _transport.Send(new ProtocolOutput(
            [targetId],
            "analyzer_configuration",
            [
                new Atom(AtomType.Integer, 1, 0, null),
                new Atom(AtomType.Integer, (long)sourceInstanceId.Value, 0, null),
                new Atom(AtomType.Float, 0, sampleRate, null)
            ],
            DeliverySemantics.ActivePresentation));
    }

    private ulong GetSpectrumRecipient(InstanceId sourceInstanceId)
    {
        var viewerId = Volatile.Read(ref _activeViewerId);
        return GetFocusedBank(viewerId)?.InstanceId == sourceInstanceId
            ? viewerId
            : 0;
    }

    private sealed class AudioCapture
    {
        private const int ChannelCount = 4;
        private readonly double[][] _channels;
        private readonly int _capacity;
        private int _writeIndex;
        private int _readIndex;

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

            return count - writable;
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

using System.Collections.Concurrent;
using System.Diagnostics;
using System.Numerics;

using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.Core.Topology;
using Consolidator.Managed.Protocol.Notifications;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Analysis;

public sealed class FftAnalyzer :
    IInstanceAudioInputService,
    IInstancePreparationHandler,
    IAnalyzerLifecycle,
    IDisposable
{
    private const int FftSize = 1024;
    private const int QueueLength = 4;
    private const int SpectrumBinCount = FftSize / 2 + 1;
    private const int SpectrumIntervalMilliseconds = 33;
    private const double DefaultSampleRate = 48000.0;
    private static readonly TimeSpan SpectrumInterval =
        TimeSpan.FromMilliseconds(SpectrumIntervalMilliseconds);

    private readonly ConcurrentDictionary<InstanceId, AudioCapture> _captures = new();
    private readonly ConcurrentDictionary<InstanceId, BankAddress?> _focusedBanks = new();
    private readonly ConcurrentDictionary<InstanceId, Preparation> _preparations = new();
    private readonly TopologyIndex _topology;
    private readonly SpectrumPublisher _spectrumPublisher;
    private readonly StateTopologyObserver _topologyObserver;
    private readonly CancellationTokenSource _cancellation = new();
    private readonly Task _worker;
    private readonly double[] _window = SpectrumProcessor.CreateWindow(FftSize);
    private readonly Complex[] _groupFft = new Complex[FftSize];
    private readonly float[] _groupSpectrum = new float[SpectrumBinCount];
    private ulong _activeViewerId;
    private CaptureDemand _captureDemand = CaptureDemand.Empty;
    private int _disposeStarted;

    internal FftAnalyzer(
        StateTopologyObserver topology,
        TopologyIndex topologyIndex,
        IPresentationTransport transport)
    {
        ArgumentNullException.ThrowIfNull(topology);
        ArgumentNullException.ThrowIfNull(topologyIndex);
        ArgumentNullException.ThrowIfNull(transport);

        _topologyObserver = topology;
        _topology = topologyIndex;
        _spectrumPublisher = new SpectrumPublisher(transport);
        topology.FocusedBankChangedEvent += FocusedBankChanged;
        topology.TopologyChangedEvent += TopologyChanged;
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
            _captures[instanceId] = CreateCapture(preparation);
        }

    }

    public void RemoveInstance(InstanceId instanceId)
    {
        _captures.TryRemove(instanceId, out _);
        _preparations.TryRemove(instanceId, out _);
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

            RebuildCaptureDemand();
            return;
        }

        if (Interlocked.CompareExchange(
            ref _activeViewerId,
            0,
            instanceId.Value) != instanceId.Value)
        {
            return;
        }

        SetCaptureDemand(CaptureDemand.Empty);
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

    internal EqualizerCurve AnalyzeEqualizerResponse(
        InstanceId instanceId,
        IReadOnlyList<EqualizerFilterSnapshot> filters)
    {
        var sampleRate = _preparations.TryGetValue(instanceId, out var preparation)
            ? preparation.SampleRate
            : DefaultSampleRate;
        var frequencies = EqualizerResponseAnalyzer.CreateApproximationFrequencies(sampleRate);
        return EqualizerResponseAnalyzer.CalculateAtFrequencies(
            filters,
            frequencies,
            sampleRate);
    }

    public void Dispose()
    {
        if (Interlocked.Exchange(ref _disposeStarted, 1) != 0)
        {
            return;
        }

        _topologyObserver.FocusedBankChangedEvent -= FocusedBankChanged;
        _topologyObserver.TopologyChangedEvent -= TopologyChanged;
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
        if (focusedBank is null)
        {
            _focusedBanks.TryRemove(instanceId, out _);
            if (Interlocked.CompareExchange(
                ref _activeViewerId,
                0,
                instanceId.Value) == instanceId.Value)
            {
                SetCaptureDemand(CaptureDemand.Empty);
            }
            return;
        }

        _focusedBanks[instanceId] = focusedBank;
        if (IsViewerActive(instanceId))
        {
            RebuildCaptureDemand();
        }
    }

    private void TopologyChanged()
    {
        if (Volatile.Read(ref _activeViewerId) != 0)
        {
            RebuildCaptureDemand();
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
                    SpectrumInterval)
            {
                ProcessNextCapture();
                lastSpectrumProcessing = timestamp;
            }

            await Task.Delay(15, _cancellation.Token);
        }
    }

    private void ProcessNextCapture()
    {
        var demand = Volatile.Read(ref _captureDemand);
        if (demand.SourceInstanceId.Value == 0)
        {
            return;
        }

        if (!_captures.TryGetValue(demand.SourceInstanceId, out var capture))
        {
            return;
        }

        if (!capture.TryReadWindow(_window, FftSize))
        {
            capture.FillSilenceWindow(FftSize);
        }
        AnalyzeWindow(demand, capture);
    }

    private void AnalyzeWindow(
        CaptureDemand demand,
        AudioCapture capture)
    {
        if (demand.HasGroup)
        {
            Array.Copy(capture.Fft, _groupFft, FftSize);
        }

        foreach (var peerInstanceId in demand.PeerInstanceIds)
        {
            if (!_captures.TryGetValue(peerInstanceId, out var peerCapture))
            {
                continue;
            }

            if (!peerCapture.TryReadWindow(_window, FftSize))
            {
                peerCapture.FillSilenceWindow(FftSize);
            }

            for (var index = 0; index < FftSize; index++)
            {
                _groupFft[index] += peerCapture.Fft[index];
            }
        }

        SpectrumProcessor.Process(capture.Fft, capture.Spectrum, FftSize);
        float[]? groupSpectrum = null;
        if (demand.HasGroup)
        {
            SpectrumProcessor.Process(_groupFft, _groupSpectrum, FftSize);
            groupSpectrum = _groupSpectrum;
        }

        float[]? referenceSpectrum = null;
        if (capture.ReferenceConnected)
        {
            SpectrumProcessor.Process(capture.ReferenceFft, capture.ReferenceSpectrum, FftSize);
            referenceSpectrum = capture.ReferenceSpectrum;
        }

        _spectrumPublisher.Publish(
            GetSpectrumRecipient(demand),
            demand.SourceInstanceId,
            capture.Spectrum,
            groupSpectrum,
            referenceSpectrum);
    }

    private readonly record struct Preparation(
        double SampleRate,
        nuint MaximumFrameCount);

    private bool IsViewerActive(InstanceId instanceId)
    {
        return Volatile.Read(ref _activeViewerId) == instanceId.Value;
    }

    private bool IsSourceDemanded(InstanceId instanceId)
    {
        return Volatile.Read(ref _captureDemand).Contains(instanceId);
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
        var preparation = _preparations.TryGetValue(
            sourceInstanceId,
            out var current)
                ? current
                : new Preparation(DefaultSampleRate, FftSize);
        _captures.GetOrAdd(
            sourceInstanceId,
            _ => CreateCapture(preparation));
    }

    private static AudioCapture CreateCapture(Preparation preparation)
    {
        var blockSize = preparation.MaximumFrameCount != 0
            ? (int)Math.Min(preparation.MaximumFrameCount, 8192)
            : FftSize;
        return new AudioCapture(
            blockSize,
            QueueLength,
            preparation.SampleRate,
            SpectrumInterval);
    }

    private void RebuildCaptureDemand()
    {
        var viewerId = Volatile.Read(ref _activeViewerId);
        var focusedBank = GetFocusedBank(viewerId);
        if (focusedBank is null)
        {
            SetCaptureDemand(CaptureDemand.Empty);
            return;
        }

        var sourceInstanceId = focusedBank.Value.InstanceId;
        var hasGroup = _topology.TryGetBankGroup(focusedBank.Value, out _);
        var peerInstanceIds = hasGroup
            ? _topology.GetConnectedBankPeers(focusedBank.Value)
                .Select(bank => bank.InstanceId)
                .Where(instanceId => instanceId != sourceInstanceId)
                .Distinct()
                .OrderBy(instanceId => instanceId.Value)
                .ToArray()
            : [];
        SetCaptureDemand(new CaptureDemand(
            sourceInstanceId,
            [sourceInstanceId, .. peerInstanceIds],
            peerInstanceIds,
            hasGroup));
    }

    private void SetCaptureDemand(CaptureDemand demand)
    {
        foreach (var instanceId in demand.InstanceIds)
        {
            PrepareCapture(instanceId);
        }

        var previous = Interlocked.Exchange(ref _captureDemand, demand);
        foreach (var instanceId in previous.InstanceIds)
        {
            if (!demand.InstanceIds.Contains(instanceId))
            {
                _captures.TryRemove(instanceId, out _);
            }
        }
    }

    private ulong GetSpectrumRecipient(CaptureDemand demand)
    {
        var viewerId = Volatile.Read(ref _activeViewerId);
        return ReferenceEquals(Volatile.Read(ref _captureDemand), demand)
            ? viewerId
            : 0;
    }

    private sealed record CaptureDemand(
        InstanceId SourceInstanceId,
        InstanceId[] InstanceIds,
        InstanceId[] PeerInstanceIds,
        bool HasGroup)
    {
        public static CaptureDemand Empty { get; } = new(
            new InstanceId(0),
            [],
            [],
            false);

        public bool Contains(InstanceId instanceId)
        {
            foreach (var demandedInstanceId in InstanceIds)
            {
                if (demandedInstanceId == instanceId)
                {
                    return true;
                }
            }

            return false;
        }
    }
}

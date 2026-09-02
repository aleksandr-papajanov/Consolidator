using System.Collections.Concurrent;
using System.Diagnostics;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.Protocol.Transport;
using Consolidator.Managed.Protocol.Notifications;

namespace Consolidator.Managed.Analysis;

public sealed class FftAnalyzer :
    IInstanceAudioInputService,
    IInstancePreparationHandler,
    IAnalyzerLifecycle,
    IDisposable
{
    private const int FftSize = 1024;
    private const int HopSize = FftSize / 2;
    private const int QueueLength = 4;
    private const int SpectrumBinCount = FftSize / 2 + 1;
    private const int SpectrumIntervalMilliseconds = 33;
    private const double DefaultSampleRate = 48000.0;

    private readonly ConcurrentDictionary<InstanceId, AudioCapture> _captures = new();
    private readonly ConcurrentDictionary<InstanceId, BankAddress?> _focusedBanks = new();
    private readonly ConcurrentDictionary<InstanceId, Preparation> _preparations = new();
    private readonly SpectrumPublisher _spectrumPublisher;
    private ulong _activeViewerId;
    private ulong _activeSourceId;
    private readonly StateTopologyObserver _topologyObserver;
    private readonly CancellationTokenSource _cancellation = new();
    private readonly Task _worker;
    private readonly double[] _window = SpectrumProcessor.CreateWindow(FftSize);
    private int _disposeStarted;

    internal FftAnalyzer(
        StateTopologyObserver topology,
        IPresentationTransport transport)
    {
        ArgumentNullException.ThrowIfNull(topology);
        ArgumentNullException.ThrowIfNull(transport);

        _topologyObserver = topology;
        _spectrumPublisher = new SpectrumPublisher(transport, GetSpectrumRecipient);
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

            var previousBank = GetFocusedBank(previousViewerId);
            var focusedBank = GetFocusedBank(instanceId.Value);
            Volatile.Write(
                ref _activeSourceId,
                focusedBank?.InstanceId.Value ?? 0);
            StopCapture(previousBank, focusedBank);
            if (focusedBank is { } bank)
            {
                PrepareCapture(bank.InstanceId);
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

        if (capture.TryReadWindow(_window, FftSize, HopSize))
        {
            AnalyzeWindow(sourceInstanceId, capture);
        }
    }

    private void AnalyzeWindow(
        InstanceId sourceInstanceId,
        AudioCapture capture)
    {
        SpectrumProcessor.Process(capture, FftSize);
        _spectrumPublisher.Publish(
            sourceInstanceId,
            capture.MainSpectrum,
            capture.ReferenceSpectrum);
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

    private ulong GetSpectrumRecipient(InstanceId sourceInstanceId)
    {
        var viewerId = Volatile.Read(ref _activeViewerId);
        return GetFocusedBank(viewerId)?.InstanceId == sourceInstanceId
            ? viewerId
            : 0;
    }

}

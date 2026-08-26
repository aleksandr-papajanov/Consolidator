using System.Diagnostics;
using System.Numerics;
using MathNet.Numerics.IntegralTransforms;
using Consolidator.Managed.Core.Services;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State;
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
    private const int CurveIntervalMilliseconds = 33;
    private const int CurveUpdatesPerInterval = 2;
    private const int SpectrumIntervalMilliseconds = 33;
    private const double MinimumDecibels = -90.0;

    private readonly AnalyzerRegistry _registry;
    private readonly System.Collections.Concurrent.ConcurrentDictionary<InstanceId, BankAddress?> _focusedBanks = new();
    private readonly System.Collections.Concurrent.ConcurrentDictionary<InstanceId, BankAddress> _pendingCurveBanks = new();
    private readonly System.Collections.Concurrent.ConcurrentDictionary<InstanceId, Preparation> _preparations = new();
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
        IPresentationTransport transport,
        AnalyzerRegistry registry)
    {
        ArgumentNullException.ThrowIfNull(topology);
        ArgumentNullException.ThrowIfNull(transport);
        ArgumentNullException.ThrowIfNull(registry);

        _transport = transport;
        _registry = registry;
        _topologyObserver = topology;
        topology.FocusedBankChangedEvent += FocusedBankChanged;
        registry.CurveChanged += CurveChanged;
        _worker = Task.Run(ProcessCapturesAsync);
    }

    public void Prepare(
        InstanceId instanceId,
        double sampleRate,
        nuint maximumFrameCount)
    {
        if (sampleRate <= 0)
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
            _registry.PrepareCapture(instanceId, blockSize, QueueLength);
        }

        var focusedBank = GetFocusedBank(Volatile.Read(ref _activeViewerId));
        if (focusedBank is { } bank && bank.InstanceId == instanceId)
        {
            RequestCurvePresentation(bank);
        }
    }

    public void ReplayEqualizerPresentation(InstanceId instanceId)
    {
        if (IsViewerActive(instanceId) &&
            _focusedBanks.TryGetValue(instanceId, out var focusedBank) &&
            focusedBank is { } bank)
        {
            RequestCurvePresentation(bank);
        }
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
                RequestCurvePresentation(bank);
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
            !_registry.TryGetCapture(instanceId, out var capture))
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
        _registry.CurveChanged -= CurveChanged;
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
            _preparations.TryRemove(instanceId, out _);
            _pendingCurveBanks.TryRemove(instanceId, out _);
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
            RequestCurvePresentation(focusedBank.Value);
        }
    }

    private void CurveChanged(
        InstanceId instanceId,
        int bankIndex,
        AnalyzerRegistry.CurveKind kind)
    {
        RequestCurvePresentation(new BankAddress(instanceId, bankIndex));
    }

    private void PublishAllCurvePresentations(InstanceId instanceId, int bankIndex)
    {
        PublishCurvePresentation(instanceId, bankIndex, AnalyzerRegistry.CurveKind.Equalizer);
        PublishCurvePresentation(instanceId, bankIndex, AnalyzerRegistry.CurveKind.CompressorDetector);
        PublishCurvePresentation(instanceId, bankIndex, AnalyzerRegistry.CurveKind.SaturatorDetector);
    }

    private void PublishCurvePresentation(
        InstanceId sourceInstanceId,
        int bankIndex,
        AnalyzerRegistry.CurveKind kind)
    {
        var targetId = GetCurveRecipient(sourceInstanceId, bankIndex, kind);
        if (targetId == 0 ||
            !_registry.TryGetCurves(sourceInstanceId, bankIndex, kind, out var curves))
        {
            return;
        }

        _transport.Send(new ProtocolOutput(
            [targetId],
            GetCurveSelector(kind),
            CreateEqualizerCurveAtoms(curves),
            DeliverySemantics.LatestAnalysis));
        RuntimeMetrics.Shared.ForInstance(sourceInstanceId.Value)
            .RecordCurveFrame();
    }

    private static string GetCurveSelector(AnalyzerRegistry.CurveKind kind)
    {
        return kind switch
        {
            AnalyzerRegistry.CurveKind.Equalizer => "equalizer_curves",
            AnalyzerRegistry.CurveKind.CompressorDetector => "compressor_detector_curves",
            AnalyzerRegistry.CurveKind.SaturatorDetector => "saturator_detector_curves",
            _ => throw new ArgumentOutOfRangeException(nameof(kind))
        };
    }

    private static List<Atom> CreateEqualizerCurveAtoms(
        AnalyzerRegistry.EqualizerCurves curves)
    {
        var atomCount = 3 + curves.Combined.Count + curves.AllBanks.Count;
        foreach (var filter in curves.Filters)
        {
            atomCount += filter.Values.Count + 1;
        }

        var atoms = new List<Atom>(atomCount)
        {
            new(AtomType.Integer, 1, 0, null),
            new(AtomType.Integer, curves.Active ? 1 : 0, 0, null),
            new(AtomType.Integer, curves.Filters.Count, 0, null)
        };

        foreach (var filter in curves.Filters)
        {
            atoms.Add(new Atom(AtomType.Integer, filter.Active ? 1 : 0, 0, null));
            foreach (var value in filter.Values)
            {
                atoms.Add(new Atom(AtomType.Float, 0, value, null));
            }
        }

        foreach (var value in curves.Combined)
        {
            atoms.Add(new Atom(AtomType.Float, 0, value, null));
        }
        foreach (var value in curves.AllBanks)
        {
            atoms.Add(new Atom(AtomType.Float, 0, value, null));
        }
        return atoms;
    }

    private async Task ProcessCapturesAsync()
    {
        var lastCurveProcessing = 0L;
        var lastSpectrumProcessing = 0L;
        while (!_cancellation.IsCancellationRequested)
        {
            var timestamp = Stopwatch.GetTimestamp();
            if (lastCurveProcessing == 0 ||
                Stopwatch.GetElapsedTime(lastCurveProcessing, timestamp) >=
                    TimeSpan.FromMilliseconds(CurveIntervalMilliseconds))
            {
                _registry.ProcessDirtyBanks(
                    CurveUpdatesPerInterval,
                    HasCurveRecipient);
                ProcessPendingCurvePresentations();
                lastCurveProcessing = timestamp;
            }

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

    private void RequestCurvePresentation(BankAddress bank)
    {
        if (GetCurveRecipient(
                bank.InstanceId,
                bank.BankIndex,
                AnalyzerRegistry.CurveKind.Equalizer) == 0 &&
            GetCurveRecipient(
                bank.InstanceId,
                bank.BankIndex,
                AnalyzerRegistry.CurveKind.CompressorDetector) == 0 &&
            GetCurveRecipient(
                bank.InstanceId,
                bank.BankIndex,
                AnalyzerRegistry.CurveKind.SaturatorDetector) == 0)
        {
            return;
        }
        _pendingCurveBanks[bank.InstanceId] = bank;
    }

    private void ProcessPendingCurvePresentations()
    {
        foreach (var pair in _pendingCurveBanks.ToArray())
        {
            if (!_pendingCurveBanks.TryRemove(pair.Key, out var bank))
            {
                continue;
            }
            PublishAllCurvePresentations(bank.InstanceId, bank.BankIndex);
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
        if (!_registry.TryGetCapture(sourceInstanceId, out var capture))
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

    private bool HasCurveRecipient(
        InstanceId sourceInstanceId,
        int bankIndex,
        AnalyzerRegistry.CurveKind kind)
    {
        return GetCurveRecipient(sourceInstanceId, bankIndex, kind) != 0;
    }

    private void AnalyzeWindow(
        InstanceId sourceInstanceId,
        AnalyzerRegistry.AudioCapture capture)
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
        _registry.PrepareCapture(sourceInstanceId, blockSize, QueueLength);
    }

    private void StopCapture(BankAddress? previous, BankAddress? current)
    {
        if (previous is { } previousBank &&
            previousBank.InstanceId != current?.InstanceId)
        {
            _registry.RemoveCapture(previousBank.InstanceId);
        }
    }

    private ulong GetCurveRecipient(
        InstanceId sourceInstanceId,
        int bankIndex,
        AnalyzerRegistry.CurveKind kind)
    {
        var viewerId = Volatile.Read(ref _activeViewerId);
        var focusedBank = GetFocusedBank(viewerId);
        return focusedBank?.InstanceId == sourceInstanceId &&
            (kind != AnalyzerRegistry.CurveKind.Equalizer ||
                focusedBank.Value.BankIndex == bankIndex)
                    ? viewerId
                    : 0;
    }

    private ulong GetSpectrumRecipient(InstanceId sourceInstanceId)
    {
        var viewerId = Volatile.Read(ref _activeViewerId);
        return GetFocusedBank(viewerId)?.InstanceId == sourceInstanceId
            ? viewerId
            : 0;
    }
}

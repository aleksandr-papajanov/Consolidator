using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.Protocol.Notifications;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Native;

internal sealed class DspTelemetryService :
    IDspTelemetryLifecycle,
    IInputLevelerTelemetry,
    IDisposable
{
    private const int PublicationIntervalMilliseconds = 33;
    private readonly object _lock = new();
    private readonly Dictionary<InstanceId, nint> _sources = new();
    private readonly Dictionary<InstanceId, uint> _trackLevelSequences = new();
    private readonly Dictionary<InstanceId, BankAddress?> _focusedBanks = new();
    private readonly StateTopologyObserver _topologyObserver;
    private readonly DspTelemetryPublisher _publisher;
    private readonly CancellationTokenSource _cancellation = new();
    private readonly Task _worker;
    private InstanceId? _activeViewer;
    private uint _lastPublishedSequence;
    private uint _trackLevelRevision;
    private int _disposeStarted;

    public DspTelemetryService(
        StateTopologyObserver topologyObserver,
        IPresentationTransport transport)
    {
        ArgumentNullException.ThrowIfNull(topologyObserver);
        ArgumentNullException.ThrowIfNull(transport);
        _topologyObserver = topologyObserver;
        _publisher = new DspTelemetryPublisher(transport, GetRecipient);
        _topologyObserver.FocusedBankChangedEvent += FocusedBankChanged;
        _worker = Task.Run(ProcessAsync);
    }

    public unsafe void RegisterSource(
        InstanceId instanceId,
        DspStateExchange* exchange)
    {
        if (exchange == null)
        {
            throw new ArgumentNullException(nameof(exchange));
        }

        lock (_lock)
        {
            _sources[instanceId] = (nint)exchange;
        }
    }

    public void RemoveInstance(InstanceId instanceId)
    {
        lock (_lock)
        {
            _sources.Remove(instanceId);
            _trackLevelSequences.Remove(instanceId);
            _focusedBanks.Remove(instanceId);
            if (_activeViewer == instanceId)
            {
                _activeViewer = null;
                _lastPublishedSequence = 0;
            }
        }
    }

    public void SetInstanceActive(InstanceId instanceId, bool active)
    {
        lock (_lock)
        {
            if (active)
            {
                _activeViewer = instanceId;
                _lastPublishedSequence = 0;
                _trackLevelSequences.Clear();
                return;
            }

            if (_activeViewer == instanceId)
            {
                _activeViewer = null;
                _lastPublishedSequence = 0;
                _trackLevelSequences.Clear();
            }
        }
    }

    public unsafe bool TryReadSlowLevel(
        InstanceId instanceId,
        out float levelDb)
    {
        lock (_lock)
        {
            levelDb = 0.0F;
            if (!_sources.TryGetValue(instanceId, out var source) ||
                !DspTelemetryReader.TryRead(
                    (DspStateExchange*)source,
                    out var snapshot) ||
                ((DspTelemetryValues)snapshot.ValuesPresent &
                    DspTelemetryValues.InputLevel) == 0 ||
                !float.IsFinite(snapshot.InputLevelDb))
            {
                return false;
            }

            levelDb = snapshot.InputLevelDb;
            return true;
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

        lock (_lock)
        {
            _sources.Clear();
            _trackLevelSequences.Clear();
            _focusedBanks.Clear();
            _activeViewer = null;
        }
        _cancellation.Dispose();
    }

    private async Task ProcessAsync()
    {
        while (!_cancellation.IsCancellationRequested)
        {
            PublishLatest();
            await Task.Delay(
                PublicationIntervalMilliseconds,
                _cancellation.Token);
        }
    }

    private unsafe void PublishLatest()
    {
        InstanceId? sourceInstanceId = null;
        DspTelemetrySnapshot focusedSnapshot = default;
        InstanceId? recipient = null;
        var trackLevels = new List<TrackLevelTelemetry>();
        var trackLevelsChanged = false;
        uint trackLevelRevision = 0;

        lock (_lock)
        {
            if (_activeViewer is not { } viewer)
            {
                return;
            }

            recipient = viewer;
            foreach (var (instanceId, source) in _sources)
            {
                if (!DspTelemetryReader.TryRead(
                        (DspStateExchange*)source,
                        out var snapshot) ||
                    ((DspTelemetryValues)snapshot.ValuesPresent &
                        DspTelemetryValues.OutputLevel) == 0 ||
                    !float.IsFinite(snapshot.OutputLevelDb))
                {
                    continue;
                }

                if (!_trackLevelSequences.TryGetValue(
                        instanceId,
                        out var lastSequence) ||
                    lastSequence != snapshot.Sequence)
                {
                    trackLevelsChanged = true;
                }
                _trackLevelSequences[instanceId] = snapshot.Sequence;
                trackLevels.Add(new TrackLevelTelemetry(
                    instanceId,
                    snapshot.OutputLevelDb));
            }

            if (trackLevelsChanged)
            {
                trackLevelRevision = ++_trackLevelRevision;
            }

            if (_focusedBanks.TryGetValue(viewer, out var focusedBank) &&
                focusedBank is { } bank &&
                _sources.TryGetValue(bank.InstanceId, out var focusedSource) &&
                DspTelemetryReader.TryRead(
                    (DspStateExchange*)focusedSource,
                    out focusedSnapshot) &&
                focusedSnapshot.Sequence != _lastPublishedSequence)
            {
                _lastPublishedSequence = focusedSnapshot.Sequence;
                sourceInstanceId = bank.InstanceId;
            }
        }

        if (recipient is { } trackLevelRecipient && trackLevelsChanged)
        {
            _publisher.PublishTrackLevels(
                trackLevelRecipient,
                trackLevelRevision,
                trackLevels);
        }

        if (sourceInstanceId is { } inputSource &&
            ((DspTelemetryValues)focusedSnapshot.ValuesPresent &
                DspTelemetryValues.InputLevel) != 0)
        {
            _publisher.PublishInputLevel(
                inputSource,
                focusedSnapshot.Sequence,
                focusedSnapshot.InputLevelDb,
                focusedSnapshot.InputLevelFastDb);
        }
    }

    private void FocusedBankChanged(
        InstanceId instanceId,
        BankAddress? focusedBank)
    {
        lock (_lock)
        {
            if (focusedBank is { })
            {
                _focusedBanks[instanceId] = focusedBank;
            }
            else
            {
                _focusedBanks.Remove(instanceId);
            }

            if (_activeViewer == instanceId)
            {
                _lastPublishedSequence = 0;
            }
        }
    }

    private ulong GetRecipient(InstanceId sourceInstanceId)
    {
        lock (_lock)
        {
            if (_activeViewer is not { } viewer ||
                !_focusedBanks.TryGetValue(viewer, out var focusedBank))
            {
                return 0;
            }

            return focusedBank?.InstanceId == sourceInstanceId
                ? viewer.Value
                : 0;
        }
    }
}

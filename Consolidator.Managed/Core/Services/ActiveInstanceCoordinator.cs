using Consolidator.Managed.Core.Services.Abstractions;

namespace Consolidator.Managed.Core.Services;

internal sealed class ActiveInstanceCoordinator
{
    private readonly IAnalyzerLifecycle _analyzer;
    private readonly IDspTelemetryLifecycle _telemetry;
    private readonly IInstancePresentationLifecycle _presentation;
    private readonly IRegistryChangeSink _registryChanges;
    private readonly object _lock = new();
    private InstanceId? _activeInstanceId;

    public ActiveInstanceCoordinator(
        IAnalyzerLifecycle analyzer,
        IDspTelemetryLifecycle telemetry,
        IInstancePresentationLifecycle presentation,
        IRegistryChangeSink registryChanges)
    {
        ArgumentNullException.ThrowIfNull(analyzer);
        ArgumentNullException.ThrowIfNull(telemetry);
        ArgumentNullException.ThrowIfNull(presentation);
        ArgumentNullException.ThrowIfNull(registryChanges);

        _analyzer = analyzer;
        _telemetry = telemetry;
        _presentation = presentation;
        _registryChanges = registryChanges;
    }

    public void SetInstanceActive(InstanceId instanceId, bool active)
    {
        lock (_lock)
        {
            if (!active)
            {
                if (_activeInstanceId != instanceId)
                {
                    _presentation.SetActive(instanceId.Value, false);
                    _registryChanges.UnregisterObserver(instanceId.Value);
                    return;
                }

                _analyzer.SetInstanceActive(instanceId, false);
                _telemetry.SetInstanceActive(instanceId, false);
                _presentation.SetActive(instanceId.Value, false);
                _registryChanges.UnregisterObserver(instanceId.Value);
                _activeInstanceId = null;
                return;
            }

            if (_activeInstanceId is { } previous && previous != instanceId)
            {
                _analyzer.SetInstanceActive(previous, false);
                _telemetry.SetInstanceActive(previous, false);
                _presentation.SetActive(previous.Value, false);
                _registryChanges.UnregisterObserver(previous.Value);
            }

            _presentation.SetActive(instanceId.Value, true);
            _analyzer.SetInstanceActive(instanceId, true);
            _telemetry.SetInstanceActive(instanceId, true);
            _activeInstanceId = instanceId;
        }
    }

    public void Unregister(ulong instanceId)
    {
        lock (_lock)
        {
            if (_activeInstanceId?.Value == instanceId)
            {
                _analyzer.SetInstanceActive(new InstanceId(instanceId), false);
                _telemetry.SetInstanceActive(new InstanceId(instanceId), false);
                _activeInstanceId = null;
            }

            _presentation.Unregister(instanceId);
        }
    }
}

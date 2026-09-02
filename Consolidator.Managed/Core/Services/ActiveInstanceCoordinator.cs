using Consolidator.Managed.Analyzer;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Protocol.Notifications;
using Consolidator.Managed.Protocol.Transport;

namespace Consolidator.Managed.Core.Services;

internal sealed class ActiveInstanceCoordinator
{
    private readonly FftAnalyzer _analyzer;
    private readonly IPresentationTransport _presentation;
    private readonly RegistryChangePublisher _registryChanges;
    private readonly object _lock = new();
    private InstanceId? _activeInstanceId;

    public ActiveInstanceCoordinator(
        FftAnalyzer analyzer,
        IPresentationTransport presentation,
        RegistryChangePublisher registryChanges)
    {
        ArgumentNullException.ThrowIfNull(analyzer);
        ArgumentNullException.ThrowIfNull(presentation);
        ArgumentNullException.ThrowIfNull(registryChanges);

        _analyzer = analyzer;
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

                _presentation.SetActive(instanceId.Value, false);
                _registryChanges.UnregisterObserver(instanceId.Value);
                _analyzer.SetInstanceActive(instanceId, false);
                _activeInstanceId = null;
                return;
            }

            if (_activeInstanceId is { } previous && previous != instanceId)
            {
                _presentation.SetActive(previous.Value, false);
                _registryChanges.UnregisterObserver(previous.Value);
            }

            _presentation.SetActive(instanceId.Value, true);
            _analyzer.SetInstanceActive(instanceId, true);
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
                _activeInstanceId = null;
            }

            _presentation.Unregister(instanceId);
        }
    }
}
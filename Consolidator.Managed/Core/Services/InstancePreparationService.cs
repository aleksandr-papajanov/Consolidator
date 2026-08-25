namespace Consolidator.Managed.Core.Services;

using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Analyzer;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.Services.Instances;

public sealed class InstancePreparationService : IInstancePreparationService
{
    private readonly InstanceRegistry _instanceRegistry;
    private readonly IOperationGate _operationGate;
    private readonly AnalyzerRegistry _analyzerRegistry;
    private readonly IReadOnlyList<IInstancePreparationHandler> _handlers;

    public InstancePreparationService(
        InstanceRegistry instanceRegistry,
        IOperationGate operationGate,
        IEnumerable<IInstancePreparationHandler> handlers,
        AnalyzerRegistry analyzerRegistry)
    {
        _instanceRegistry = instanceRegistry;
        _operationGate = operationGate;
        _analyzerRegistry = analyzerRegistry;
        _handlers = handlers.ToArray();
    }

    public void Prepare(
        InstanceId instanceId,
        double sampleRate,
        nuint maximumFrameCount)
    {
        using (_operationGate.Enter())
        {
            if (!_instanceRegistry.Contains(instanceId))
            {
                return;
            }

            _analyzerRegistry.SetSampleRate(instanceId, sampleRate);
            foreach (var handler in _handlers)
            {
                handler.Prepare(instanceId, sampleRate, maximumFrameCount);
            }
        }
    }
}





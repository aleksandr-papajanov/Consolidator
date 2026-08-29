using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IProcessorStatusSink
{
    void ProcessorStatusChanged(InstanceId instanceId, ProcessorStatus status);
}

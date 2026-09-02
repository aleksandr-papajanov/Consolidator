namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IActivityStatusSink
{
    void BankActivityChanged(
        InstanceId instanceId,
        int bankId,
        bool active);

    void ProcessorActivityChanged(
        InstanceId instanceId,
        ProcessorStatus status);
}

using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.Services.Abstractions;

public interface IBankEffectStatusSink
{
    void BankEffectStatusChanged(
        InstanceId instanceId,
        int bankId,
        bool effectActive);
}

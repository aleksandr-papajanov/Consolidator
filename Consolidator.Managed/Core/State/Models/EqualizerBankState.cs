using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class EqualizerBankState
{
    public EqualizerBankState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        DspRuntimeState runtime,
        int bankIndex)
    {
        Bypass = values.CreateBankValue(
            instanceId,
            path.Append(StateNodeIds.Bypass),
            false,
            StateValueEditMode.CopyValue,
            observers: [new StateProjectionObserver<bool>(
                value => runtime.SetEqualizerBankActive(bankIndex, !value))]);
        Solo = values.CreateBankValue(instanceId, path.Append(StateNodeIds.Solo), false, StateValueEditMode.CopyValue);
        Filters = Enumerable.Range(0, DspConstants.EqualizerFilterCount)
            .Select(index => new FilterState(
                instanceId,
                path.Append(StateNodeIds.Filter).Append(StateNodeIds.FilterAt(index)),
                values,
                true,
                bypass => runtime.SetEqualizerFilterActive(bankIndex, index, !bypass)))
            .ToArray();
    }

    public StateValue<bool> Bypass { get; }
    public StateValue<bool> Solo { get; }
    public FilterState[] Filters { get; }

}







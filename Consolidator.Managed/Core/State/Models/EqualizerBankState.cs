using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class EqualizerBankState
{
    private readonly ActivityObserver _activity;
    private readonly int _bankIndex;

    internal EqualizerBankState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        DspRuntimeState runtime,
        int bankIndex,
        ActivityObserver activity)
    {
        _activity = activity;
        _bankIndex = bankIndex;
        Bypass = values.CreateBankValue(
            instanceId,
            path.Append(StateNodeIds.Bypass),
            false,
            StateValueEditMode.CopyValue,
            observers: [
                new StateProjectionObserver<bool>(value => runtime.SetEqualizerBankActive(bankIndex, !value)),
                activity.ObserveBankBypass(bankIndex)
            ]);
        Solo = values.CreateBankValue(instanceId, path.Append(StateNodeIds.Solo), false, StateValueEditMode.CopyValue);
        Filters = Enumerable.Range(0, DspConstants.EqualizerFilterCount)
            .Select(index => new FilterState(
                instanceId,
                path.Append(StateNodeIds.Filter).Append(StateNodeIds.FilterAt(index)),
                values,
                true,
                bypass => runtime.SetEqualizerFilterActive(bankIndex, index, !bypass),
                FilterCatalog.Equalizer[index],
                activity,
                bankIndex,
                index))
            .ToArray();
    }

    public StateValue<bool> Bypass { get; }
    public StateValue<bool> Solo { get; }
    public FilterState[] Filters { get; }
    public bool EffectActive => _activity.BankActivity(_bankIndex);

}

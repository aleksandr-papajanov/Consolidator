using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class InstanceState
{
    internal InstanceState(
        InstanceId instanceId,
        StateValueFactory values,
        DspRuntimeState runtime,
        AudibilityObserver audibilityObserver,
        StateTopologyObserver topologyObserver)
    {
        ArgumentNullException.ThrowIfNull(values);
        ArgumentNullException.ThrowIfNull(runtime);
        ArgumentNullException.ThrowIfNull(audibilityObserver);
        ArgumentNullException.ThrowIfNull(topologyObserver);

        InstanceId = instanceId;
        var instancePath = new StatePath([StateNodeIds.Instance]);
        Label = values.CreateValueWithoutHistory(
            instanceId,
            instancePath.Append(StateNodeIds.Label),
            string.Empty,
            StateValueEditMode.CopyValue);
        Mute = values.CreateValueWithoutHistory(
            instanceId,
            instancePath.Append(StateNodeIds.Mute),
            false,
            StateValueEditMode.CopyValue,
            observers: [audibilityObserver.ObserveMute(instanceId, runtime)]);
        Solo = values.CreateValueWithoutHistory(
            instanceId,
            instancePath.Append(StateNodeIds.Solo),
            false,
            StateValueEditMode.CopyValue,
            observers: [audibilityObserver.ObserveSolo(instanceId, runtime)]);
        Banks = Enumerable.Range(0, DspConstants.BankCount)
            .Select(index => new BankState(
                instanceId,
                instancePath.Append(StateNodeIds.Bank).Append(StateNodeIds.BankAt(index)),
                values,
                (BankId)index,
                topologyObserver.ObserveBankGroup(new BankAddress(instanceId, index))))
            .ToArray();
    }

    public InstanceId InstanceId { get; }

    public StateValue<string> Label { get; }
    public StateValue<bool> Mute { get; }
    public StateValue<bool> Solo { get; }
    public BankState[] Banks { get; }

}








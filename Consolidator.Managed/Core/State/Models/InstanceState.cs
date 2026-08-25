using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models;

public sealed class InstanceState
{
    private readonly object _selectionLock = new();
    private readonly StateTopologyObserver _topologyObserver;
    private BankAddress? _focusedBank;

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
        _topologyObserver = topologyObserver;
        var instancePath = new StatePath([StateNodeIds.Instance]);
        Label = values.CreateValue(
            instanceId,
            instancePath.Append(StateNodeIds.Label),
            string.Empty,
            StateValueEditMode.CopyValue);
        Mute = values.CreateValue(
            instanceId,
            instancePath.Append(StateNodeIds.Mute),
            false,
            StateValueEditMode.CopyValue,
            observers: [audibilityObserver.ObserveMute(instanceId, runtime)]);
        Solo = values.CreateValue(
            instanceId,
            instancePath.Append(StateNodeIds.Solo),
            false,
            StateValueEditMode.CopyValue,
            observers: [audibilityObserver.ObserveSolo(instanceId, runtime)]);
        _focusedBank = new BankAddress(instanceId, 0);
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

    public BankAddress? FocusedBank
    {
        get
        {
            lock (_selectionLock)
            {
                return _focusedBank;
            }
        }
        set
        {
            lock (_selectionLock)
            {
                if (_focusedBank == value)
                {
                    return;
                }
                _focusedBank = value;
            }

            _topologyObserver.FocusedBankChanged(InstanceId, value);
        }
    }

}








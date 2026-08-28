using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.Core.Topology;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed class StateTopologyObserver
{
    private readonly TopologyIndex _topology;
    private readonly StatePeerObserver _peers;

    public StateTopologyObserver(
        TopologyIndex topology,
        StatePeerObserver peers)
    {
        _topology = topology;
        _peers = peers;
    }

    public event Action<InstanceId, BankAddress?>? FocusedBankChangedEvent;

    public IStateValueObserver<GroupId?> ObserveBankGroup(BankAddress bank)
    {
        return new BankGroupObserver(this, bank);
    }

    public void AddState(InstanceState state)
    {
        ArgumentNullException.ThrowIfNull(state);
        _topology.AddInstance(
            state.InstanceId,
            state.FocusedBank,
            state.Banks.Select(bank => bank.Group.Value).ToArray());
        FocusedBankChangedEvent?.Invoke(state.InstanceId, state.FocusedBank);
        try
        {
            var affectedInstances = state.Banks
                .Where(bank => bank.Group.Value is not null)
                .SelectMany(bank => _topology.GetConnectedBankPeers(
                    new BankAddress(state.InstanceId, (int)bank.Id)))
                .Select(bank => bank.InstanceId)
                .Append(state.InstanceId)
                .Distinct()
                .ToArray();
            _peers.Refresh(affectedInstances);
        }
        catch
        {
            var affectedInstances = _topology.RemoveInstance(state.InstanceId);
            _peers.Refresh(affectedInstances);
            throw;
        }
    }

    public void RemoveState(InstanceId instanceId)
    {
        var affectedInstances = _topology.RemoveInstance(instanceId);
        FocusedBankChangedEvent?.Invoke(instanceId, null);
        _peers.Refresh(affectedInstances);
    }

    public void FocusedBankChanged(
        InstanceId instanceId,
        BankAddress? focusedBank)
    {
        _topology.UpdateFocusedBank(instanceId, focusedBank);
        FocusedBankChangedEvent?.Invoke(instanceId, focusedBank);
    }

    private void BankGroupChanged(
        BankAddress bank,
        GroupId? groupId)
    {
        var affectedInstances = _topology.UpdateBankGroup(bank, groupId);
        _peers.Refresh(affectedInstances);
    }

    private sealed class BankGroupObserver : IStateValueObserver<GroupId?>
    {
        private readonly StateTopologyObserver _owner;
        private readonly BankAddress _bank;

        public BankGroupObserver(
            StateTopologyObserver owner,
            BankAddress bank)
        {
            _owner = owner;
            _bank = bank;
        }

        public void Attach(StateValue<GroupId?> value)
        {
        }

        public void ValueChanged(
            StateValue<GroupId?> value,
            GroupId? previousValue,
            GroupId? currentValue)
        {
            _owner.BankGroupChanged(_bank, currentValue);
        }

        public void Detach(StateValue<GroupId?> value)
        {
        }
    }
}

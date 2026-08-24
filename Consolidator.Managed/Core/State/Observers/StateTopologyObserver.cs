using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.Core.Topology;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Observers;

internal sealed class StateTopologyObserver
{
    private readonly TopologyIndex _topology;
    private readonly StatePeerObserver _peers;
    private readonly AudibilityObserver _audibility;

    public StateTopologyObserver(
        TopologyIndex topology,
        StatePeerObserver peers,
        AudibilityObserver audibility)
    {
        _topology = topology;
        _peers = peers;
        _audibility = audibility;
    }

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
        try
        {
            _peers.Refresh(state.InstanceId);
            _audibility.Refresh();
        }
        catch
        {
            var affectedInstances = _topology.RemoveInstance(state.InstanceId);
            _peers.Refresh(affectedInstances);
            _audibility.Refresh();
            throw;
        }
    }

    public void RemoveState(InstanceId instanceId)
    {
        var affectedInstances = _topology.RemoveInstance(instanceId);
        _peers.Refresh(affectedInstances);
        _audibility.Refresh();
    }

    public void FocusedBankChanged(
        InstanceId instanceId,
        BankAddress? focusedBank)
    {
        _topology.UpdateFocusedBank(instanceId, focusedBank);
    }

    private void BankGroupChanged(
        BankAddress bank,
        GroupId? groupId)
    {
        var affectedInstances = _topology.UpdateBankGroup(bank, groupId);
        _peers.Refresh(affectedInstances);
        _audibility.Refresh();
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

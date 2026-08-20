using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.Topology;

public sealed class TopologyStore
{
    private readonly StateHistory _history;
    private readonly TopologyIndex _index = new();
    private readonly Dictionary<ulong, StateValue<TopologyState>> _values = new();
    private readonly object _lock = new();

    public TopologyStore(StateHistory history)
    {
        ArgumentNullException.ThrowIfNull(history);
        _history = history;
    }

    public void Register(ulong instanceId)
    {
        lock (_lock)
        {
            if (_values.ContainsKey(instanceId))
            {
                throw new InvalidOperationException(
                    $"Topology for instance {instanceId} is already registered.");
            }

            var value = _history.CreateValue(
                StateIds.Topology,
                TopologyState.Empty,
                new TopologyBinding(instanceId, _index));

            try
            {
                _values.Add(instanceId, value);
            }
            catch
            {
                value.Dispose();
                _index.Remove(instanceId);
                throw;
            }
        }
    }

    public void Unregister(ulong instanceId)
    {
        StateValue<TopologyState>? value;

        lock (_lock)
        {
            if (!_values.Remove(instanceId, out value))
            {
                return;
            }
        }

        value.Dispose();
        _index.Remove(instanceId);
    }

    public TopologyState? Get(ulong instanceId)
    {
        lock (_lock)
        {
            return _values.TryGetValue(instanceId, out var value)
                ? value.Value
                : null;
        }
    }

    public bool SetBankGroup(
        ulong instanceId,
        int bankIndex,
        GroupId? groupId)
    {
        lock (_lock)
        {
            if (!_values.TryGetValue(instanceId, out var value))
            {
                return false;
            }

            value.Value = value.Value.SetGroupId(bankIndex, groupId);
            return true;
        }
    }

    public bool SetFocusedBank(
        ulong instanceId,
        BankAddress? focusedBank)
    {
        lock (_lock)
        {
            if (!_values.TryGetValue(instanceId, out var value))
            {
                return false;
            }

            value.Value = value.Value.SetFocusedBank(focusedBank);
            return true;
        }
    }

    public IReadOnlyList<BankAddress> GetGroupMembers(BankAddress bank)
    {
        return _index.GetGroupMembers(bank);
    }

    public IReadOnlyList<ulong> GetGroupInstanceIds(GroupId groupId)
    {
        return _index.GetGroupInstanceIds(groupId);
    }

    public IReadOnlyList<BankAddress> GetGroupedBanks(ulong instanceId)
    {
        return _index.GetGroupedBanks(instanceId);
    }

    public IReadOnlyList<BankAddress> GetConnectedGroupBanks(
        IReadOnlyList<BankAddress> seeds)
    {
        return _index.GetConnectedGroupBanks(seeds);
    }
}
namespace Consolidator.Managed.Core.Topology;

internal sealed class TopologyIndex
{
    private readonly Dictionary<ulong, TopologyState> _topologies = new();
    private readonly Dictionary<BankAddress, GroupId> _bankGroups = new();
    private readonly Dictionary<GroupId, HashSet<BankAddress>> _groupBanks = new();
    private readonly Dictionary<GroupId, HashSet<ulong>> _groupInstances = new();
    private readonly Dictionary<ulong, HashSet<BankAddress>> _groupedBanks = new();
    private readonly object _lock = new();

    public void Set(
        ulong instanceId,
        TopologyState topology)
    {
        ArgumentNullException.ThrowIfNull(topology);

        lock (_lock)
        {
            if (_topologies.TryGetValue(instanceId, out var previousTopology))
            {
                RemoveIndexes(instanceId, previousTopology);
            }

            _topologies[instanceId] = topology;
            AddIndexes(instanceId, topology);
        }
    }

    public void Remove(ulong instanceId)
    {
        lock (_lock)
        {
            if (!_topologies.Remove(instanceId, out var topology))
            {
                return;
            }

            RemoveIndexes(instanceId, topology);
        }
    }

    public IReadOnlyList<BankAddress> GetGroupMembers(BankAddress bank)
    {
        lock (_lock)
        {
            if (!_bankGroups.TryGetValue(bank, out var groupId)
                || !_groupBanks.TryGetValue(groupId, out var members))
            {
                return Array.Empty<BankAddress>();
            }

            return members.ToArray();
        }
    }

    public IReadOnlyList<ulong> GetGroupInstanceIds(GroupId groupId)
    {
        lock (_lock)
        {
            return _groupInstances.TryGetValue(groupId, out var instances)
                ? instances.ToArray()
                : Array.Empty<ulong>();
        }
    }

    public IReadOnlyList<BankAddress> GetGroupedBanks(ulong instanceId)
    {
        lock (_lock)
        {
            return _groupedBanks.TryGetValue(instanceId, out var banks)
                ? banks.ToArray()
                : Array.Empty<BankAddress>();
        }
    }

    public IReadOnlyList<BankAddress> GetConnectedGroupBanks(
        IReadOnlyList<BankAddress> seeds)
    {
        ArgumentNullException.ThrowIfNull(seeds);

        lock (_lock)
        {
            var pending = new Queue<BankAddress>(seeds);
            var connected = new HashSet<BankAddress>();

            while (pending.TryDequeue(out var bank))
            {
                if (!connected.Add(bank))
                {
                    continue;
                }

                if (_groupedBanks.TryGetValue(bank.InstanceId, out var groupedBanks))
                {
                    foreach (var groupedBank in groupedBanks)
                    {
                        pending.Enqueue(groupedBank);
                    }
                }

                if (!_bankGroups.TryGetValue(bank, out var groupId)
                    || !_groupBanks.TryGetValue(groupId, out var members))
                {
                    continue;
                }

                foreach (var member in members)
                {
                    pending.Enqueue(member);
                }
            }

            return connected.ToArray();
        }
    }

    private void AddIndexes(
        ulong instanceId,
        TopologyState topology)
    {
        for (var bankIndex = 0; bankIndex < TopologyState.BankCount; bankIndex++)
        {
            var groupId = topology.GetGroupId(bankIndex);
            if (groupId is null)
            {
                continue;
            }

            var bank = new BankAddress(instanceId, bankIndex);
            _bankGroups[bank] = groupId.Value;
            AddToSet(_groupBanks, groupId.Value, bank);
            AddToSet(_groupInstances, groupId.Value, instanceId);
            AddToSet(_groupedBanks, instanceId, bank);
        }
    }

    private void RemoveIndexes(
        ulong instanceId,
        TopologyState topology)
    {
        for (var bankIndex = 0; bankIndex < TopologyState.BankCount; bankIndex++)
        {
            var groupId = topology.GetGroupId(bankIndex);
            if (groupId is null)
            {
                continue;
            }

            var bank = new BankAddress(instanceId, bankIndex);
            _bankGroups.Remove(bank);
            RemoveFromSet(_groupBanks, groupId.Value, bank);
            RemoveFromSet(_groupedBanks, instanceId, bank);

            if (_groupBanks.TryGetValue(groupId.Value, out var remainingBanks)
                && remainingBanks.Any(value => value.InstanceId == instanceId))
            {
                continue;
            }

            RemoveFromSet(_groupInstances, groupId.Value, instanceId);
        }
    }

    private static void AddToSet<TKey, TValue>(
        Dictionary<TKey, HashSet<TValue>> index,
        TKey key,
        TValue value)
        where TKey : notnull
    {
        if (!index.TryGetValue(key, out var values))
        {
            values = new HashSet<TValue>();
            index.Add(key, values);
        }

        values.Add(value);
    }

    private static void RemoveFromSet<TKey, TValue>(
        Dictionary<TKey, HashSet<TValue>> index,
        TKey key,
        TValue value)
        where TKey : notnull
    {
        if (!index.TryGetValue(key, out var values))
        {
            return;
        }

        values.Remove(value);
        if (values.Count == 0)
        {
            index.Remove(key);
        }
    }
}

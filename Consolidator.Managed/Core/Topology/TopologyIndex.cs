using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Topology;

internal sealed class TopologyIndex
{
    private readonly Dictionary<InstanceId, BankAddress?> _focusedBanks = new();
    private readonly Dictionary<BankAddress, GroupId> _bankGroups = new();
    private readonly Dictionary<GroupId, HashSet<BankAddress>> _groupBanks = new();
    private readonly Dictionary<GroupId, HashSet<InstanceId>> _groupInstances = new();
    private readonly object _lock = new();

    public void AddInstance(
        InstanceId instanceId,
        BankAddress? focusedBank,
        IReadOnlyList<GroupId?> bankGroups)
    {
        ArgumentNullException.ThrowIfNull(bankGroups);
        if (bankGroups.Count != DspConstants.BankCount)
        {
            throw new ArgumentException(
                $"Topology requires exactly {DspConstants.BankCount} banks.",
                nameof(bankGroups));
        }

        lock (_lock)
        {
            _focusedBanks.Add(instanceId, focusedBank);
            AddIndexes(instanceId, bankGroups);
        }
    }

    public void UpdateFocusedBank(
        InstanceId instanceId,
        BankAddress? focusedBank)
    {
        lock (_lock)
        {
            if (!_focusedBanks.ContainsKey(instanceId))
            {
                throw new InvalidOperationException(
                    $"Topology instance was not found: {instanceId}.");
            }

            _focusedBanks[instanceId] = focusedBank;
        }
    }


    public IReadOnlyList<InstanceId> UpdateBankGroup(
        BankAddress bank,
        GroupId? groupId)
    {
        lock (_lock)
        {
            if (!_focusedBanks.ContainsKey(bank.InstanceId))
            {
                return Array.Empty<InstanceId>();
            }

            var affectedInstances = new HashSet<InstanceId> { bank.InstanceId };

            if (_bankGroups.TryGetValue(bank, out var previousGroupId))
            {
                if (_groupInstances.TryGetValue(previousGroupId, out var previousInstances))
                {
                    affectedInstances.UnionWith(previousInstances);
                }

                RemoveFromSet(_groupBanks, previousGroupId, bank);
                if (!_groupBanks.TryGetValue(previousGroupId, out var remainingBanks)
                    || remainingBanks.All(value => value.InstanceId != bank.InstanceId))
                {
                    RemoveFromSet(_groupInstances, previousGroupId, bank.InstanceId);
                }
                _bankGroups.Remove(bank);
            }

            if (groupId is { } newGroupId)
            {
                if (_groupInstances.TryGetValue(newGroupId, out var newInstances))
                {
                    affectedInstances.UnionWith(newInstances);
                }

                _bankGroups[bank] = newGroupId;
                AddToSet(_groupBanks, newGroupId, bank);
                AddToSet(_groupInstances, newGroupId, bank.InstanceId);
            }

            return affectedInstances.ToArray();
        }
    }

    public IReadOnlyList<InstanceId> RemoveInstance(InstanceId instanceId)
    {
        lock (_lock)
        {
            if (!_focusedBanks.Remove(instanceId))
            {
                return Array.Empty<InstanceId>();
            }

            var affectedInstances = new HashSet<InstanceId> { instanceId };
            foreach (var groupInstances in _groupInstances.Values)
            {
                if (groupInstances.Contains(instanceId))
                {
                    affectedInstances.UnionWith(groupInstances);
                }
            }

            RemoveIndexes(instanceId);
            return affectedInstances.ToArray();
        }
    }

    public IReadOnlyList<InstanceId> ResolveFocusedInstanceIds(BankAddress bank)
    {
        lock (_lock)
        {
            return _focusedBanks
                .Where(entry => entry.Value == bank)
                .Select(entry => entry.Key)
                .ToArray();
        }
    }

    public BankAddress? ResolveBankAddress(InstanceId instanceId, StatePath path)
    {
        ArgumentNullException.ThrowIfNull(path);

        var bankNode = path.Nodes
            .FirstOrDefault(node => node.Value >= 100 && node.Value < 100 + DspConstants.BankCount);
        if (bankNode.Value < 100 || bankNode.Value >= 100 + DspConstants.BankCount)
        {
            return null;
        }

        return new BankAddress(instanceId, (int)bankNode.Value - 100);
    }

    public IReadOnlyList<InstanceId> ResolveFocusedInstanceIds(InstanceId targetInstanceId)
    {
        lock (_lock)
        {
            return _focusedBanks
                .Where(entry => entry.Value?.InstanceId == targetInstanceId)
                .Select(entry => entry.Key)
                .ToArray();
        }
    }

    public InstanceId? ResolveFocusedInstanceId(InstanceId sourceInstanceId)
    {
        return GetFocusedBank(sourceInstanceId)?.InstanceId;
    }

    public IReadOnlyList<BankAddress> GetConnectedBankPeers(BankAddress bank)
    {
        lock (_lock)
        {
            if (!_bankGroups.TryGetValue(bank, out var groupId)
                || !_groupBanks.TryGetValue(groupId, out var members))
            {
                return [bank];
            }

            return members.ToArray();
        }
    }

    public IReadOnlyList<InstanceId> GetBankGroupInstanceIds(BankAddress bank)
    {
        lock (_lock)
        {
            if (!_bankGroups.TryGetValue(bank, out var groupId) ||
                !_groupBanks.TryGetValue(groupId, out var members))
            {
                return Array.Empty<InstanceId>();
            }

            return members
                .Select(member => member.InstanceId)
                .Distinct()
                .OrderBy(instanceId => instanceId.Value)
                .ToArray();
        }
    }

    private BankAddress? GetFocusedBank(InstanceId instanceId)
    {
        lock (_lock)
        {
            return _focusedBanks.TryGetValue(instanceId, out var focusedBank)
                ? focusedBank
                : null;
        }
    }

    private void AddIndexes(
        InstanceId instanceId,
        IReadOnlyList<GroupId?> bankGroups)
    {
        for (var bankIndex = 0; bankIndex < DspConstants.BankCount; bankIndex++)
        {
            var groupId = bankGroups[bankIndex];
            if (groupId is null)
            {
                continue;
            }

            var bank = new BankAddress(instanceId, bankIndex);
            _bankGroups[bank] = groupId.Value;
            AddToSet(_groupBanks, groupId.Value, bank);
            AddToSet(_groupInstances, groupId.Value, instanceId);
        }
    }

    private void RemoveIndexes(InstanceId instanceId)
    {
        for (var bankIndex = 0; bankIndex < DspConstants.BankCount; bankIndex++)
        {
            var bank = new BankAddress(instanceId, bankIndex);
            if (!_bankGroups.Remove(bank, out var groupId))
            {
                continue;
            }

            RemoveFromSet(_groupBanks, groupId, bank);
            if (_groupBanks.TryGetValue(groupId, out var remainingBanks)
                && remainingBanks.Any(value => value.InstanceId == instanceId))
            {
                continue;
            }

            RemoveFromSet(_groupInstances, groupId, instanceId);
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





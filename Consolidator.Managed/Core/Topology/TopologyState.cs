namespace Consolidator.Managed.Core.Topology;

public sealed class TopologyState : IEquatable<TopologyState>
{
    public const int BankCount = 7;

    private readonly GroupId?[] _groups;
    private readonly IReadOnlyList<GroupId?> _readonlyGroups;

    public TopologyState()
        : this(new GroupId?[BankCount], null)
    {
    }

    private TopologyState(
        GroupId?[] groups,
        BankAddress? focusedBank)
    {
        if (groups.Length != BankCount)
        {
            throw new ArgumentException(
                $"Topology must contain exactly {BankCount} banks.",
                nameof(groups));
        }

        _groups = groups;
        _readonlyGroups = Array.AsReadOnly(_groups);
        FocusedBank = focusedBank;
    }

    public static TopologyState Empty { get; } = new();

    public IReadOnlyList<GroupId?> Groups => _readonlyGroups;

    public BankAddress? FocusedBank { get; }

    public GroupId? GetGroupId(int bankIndex)
    {
        ValidateBankIndex(bankIndex);
        return _groups[bankIndex];
    }

    public TopologyState SetGroupId(
        int bankIndex,
        GroupId? groupId)
    {
        ValidateBankIndex(bankIndex);

        var groups = (GroupId?[])_groups.Clone();
        groups[bankIndex] = groupId;
        return new TopologyState(groups, FocusedBank);
    }

    public TopologyState SetFocusedBank(BankAddress? focusedBank)
    {
        if (focusedBank is { } value)
        {
            ValidateBankIndex(value.BankIndex);
        }

        return new TopologyState(
            (GroupId?[])_groups.Clone(),
            focusedBank);
    }

    public bool Equals(TopologyState? other)
    {
        return other is not null
            && _groups.SequenceEqual(other._groups)
            && FocusedBank == other.FocusedBank;
    }

    public override bool Equals(object? obj)
    {
        return Equals(obj as TopologyState);
    }

    public override int GetHashCode()
    {
        var hashCode = new HashCode();

        foreach (var groupId in _groups)
        {
            hashCode.Add(groupId);
        }

        hashCode.Add(FocusedBank);

        return hashCode.ToHashCode();
    }

    private static void ValidateBankIndex(int bankIndex)
    {
        if (bankIndex is < 0 or >= BankCount)
        {
            throw new ArgumentOutOfRangeException(nameof(bankIndex));
        }
    }
}
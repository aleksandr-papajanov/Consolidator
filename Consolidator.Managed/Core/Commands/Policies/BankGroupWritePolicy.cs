using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Commands.Policies;

internal sealed class BankGroupWritePolicy : IStateWritePolicy
{
    public bool Applies(StatePath path) =>
        TryGetBankGroupIndex(path, out _);

    public bool IsAllowed(
        WriteStateCommand command,
        InstanceCommandContext context)
    {
        foreach (var entry in command.Entries)
        {
            if (!TryGetBankGroupIndex(entry.Path, out var bankIndex) ||
                entry.ValueType != typeof(GroupId?))
            {
                continue;
            }

            var bank = context.State.Instance.Banks[bankIndex];
            var currentGroup = bank.Group.Value;
            var requestedGroup = (GroupId?)entry.Value;

            if (requestedGroup?.Value == 0 || currentGroup?.Value == 0)
            {
                return false;
            }

            if (currentGroup is not null && requestedGroup is not null)
            {
                return false;
            }

            if (requestedGroup is null)
            {
                continue;
            }

            var sameGroupOnTrack = context.State.Instance.Banks.Any(candidate =>
                candidate.Id != bank.Id &&
                candidate.Group.Value?.Value == requestedGroup.Value.Value);
            var sameGroupInCommand = command.Entries.Any(candidate =>
                TryGetBankGroupIndex(candidate.Path, out var candidateBankIndex) &&
                candidateBankIndex != bankIndex &&
                candidate.ValueType == typeof(GroupId?) &&
                ((GroupId?)candidate.Value)?.Value == requestedGroup.Value.Value);
            if (sameGroupOnTrack || sameGroupInCommand)
            {
                return false;
            }
        }

        return true;
    }

    private static bool TryGetBankGroupIndex(
        StatePath path,
        out int bankIndex)
    {
        bankIndex = -1;
        var nodes = path.Nodes;
        if (nodes.Count != 4 ||
            nodes[0] != StateNodeIds.Instance ||
            nodes[1] != StateNodeIds.Bank ||
            nodes[3] != StateNodeIds.Group)
        {
            return false;
        }

        var value = (int)nodes[2].Value - 100;
        if (value < 0 || value >= DspConstants.BankCount)
        {
            return false;
        }

        bankIndex = value;
        return true;
    }
}

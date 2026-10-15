using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Commands.Definitions;

public sealed record GroupBanksCommand(IReadOnlyList<BankAddress> Banks)
    : IInstanceCommand<StateWriteStatus>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record UngroupBanksCommand(InstanceId? TargetInstanceId, int BankIndex)
    : IInstanceCommand<StateWriteStatus>, ITargetedBankCommand
{
    public CommandScope Scope => CommandScope.Coordinator;
}

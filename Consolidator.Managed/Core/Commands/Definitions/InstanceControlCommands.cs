using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Commands.Definitions;

public enum InstanceControlScope
{
    Instance,
    BankGroup
}

public enum InstanceControlSelectionMode
{
    Exclusive,
    Additive
}

public sealed record SetInstanceMuteCommand(
    InstanceControlScope TargetScope,
    bool Muted,
    InstanceControlSelectionMode Mode,
    InstanceId? TargetInstanceId) :
    IInstanceCommand<StateWriteStatus>,
    IInstanceControlCommand
{
    public CommandScope Scope => CommandScope.Coordinator;

    bool IInstanceControlCommand.RequestedValue => Muted;
}

public sealed record SetInstanceSoloCommand(
    InstanceControlScope TargetScope,
    bool Soloed,
    InstanceControlSelectionMode Mode,
    InstanceId? TargetInstanceId) :
    IInstanceCommand<StateWriteStatus>,
    IInstanceControlCommand
{
    public CommandScope Scope => CommandScope.Coordinator;

    bool IInstanceControlCommand.RequestedValue => Soloed;
}

public sealed record SetInstanceBypassCommand(
    InstanceControlScope TargetScope,
    bool Bypassed,
    InstanceControlSelectionMode Mode,
    InstanceId? TargetInstanceId) :
    IInstanceCommand<StateWriteStatus>,
    IInstanceControlCommand
{
    public CommandScope Scope => CommandScope.Coordinator;

    bool IInstanceControlCommand.RequestedValue => Bypassed;
}

public sealed record SetProcessorBypassCommand(
    ProcessorId ProcessorId,
    InstanceControlScope TargetScope,
    bool Bypassed,
    InstanceId? TargetInstanceId = null) :
    IInstanceCommand<StateWriteStatus>, ITargetedInstanceCommand
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record SetBankBypassCommand(
    InstanceControlScope TargetScope,
    bool Bypassed,
    InstanceId? TargetInstanceId,
    int BankIndex) :
    IInstanceCommand<StateWriteStatus>, ITargetedBankCommand
{
    public CommandScope Scope => CommandScope.Coordinator;
}

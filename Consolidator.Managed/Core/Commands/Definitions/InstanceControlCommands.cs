using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Commands.Definitions;

public enum InstanceControlScope
{
    Instance,
    BankGroup
}

public enum SoloSelectionMode
{
    Exclusive,
    Additive
}

public sealed record SetInstanceMuteCommand(
    InstanceControlScope TargetScope,
    bool Muted,
    SoloSelectionMode Mode,
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
    SoloSelectionMode Mode,
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
    SoloSelectionMode Mode,
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
    bool Bypassed) : IInstanceCommand<StateWriteStatus>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

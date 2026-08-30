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
    bool Muted) :
    IInstanceCommand<StateWriteStatus>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record SetInstanceSoloCommand(
    InstanceControlScope TargetScope,
    bool Soloed,
    SoloSelectionMode Mode) :
    IInstanceCommand<StateWriteStatus>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record SetProcessorBypassCommand(
    ProcessorId ProcessorId,
    InstanceControlScope TargetScope,
    bool Bypassed) : IInstanceCommand<StateWriteStatus>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record SetProcessorSoloCommand(
    ProcessorId ProcessorId,
    InstanceControlScope TargetScope,
    bool Soloed,
    SoloSelectionMode Mode) : IInstanceCommand<StateWriteStatus>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

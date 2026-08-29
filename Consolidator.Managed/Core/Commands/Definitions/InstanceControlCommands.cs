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

public sealed record InstanceControlTarget(
    InstanceId InstanceId,
    InstanceControlScope Scope,
    BankId? BankId);

public sealed record SetInstanceMuteCommand(
    InstanceControlTarget Target,
    bool Muted) :
    IInstanceCommand<StateWriteStatus>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record SetInstanceSoloCommand(
    InstanceControlTarget Target,
    bool Soloed,
    SoloSelectionMode Mode) :
    IInstanceCommand<StateWriteStatus>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record SetProcessorBypassCommand(
    ProcessorId ProcessorId,
    InstanceControlTarget Target,
    bool Bypassed) : IInstanceCommand<StateWriteStatus>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

public sealed record SetProcessorSoloCommand(
    ProcessorId ProcessorId,
    InstanceControlTarget Target,
    bool Soloed,
    SoloSelectionMode Mode) : IInstanceCommand<StateWriteStatus>
{
    public CommandScope Scope => CommandScope.Coordinator;
}

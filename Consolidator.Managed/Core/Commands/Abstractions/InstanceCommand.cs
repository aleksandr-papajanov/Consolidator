using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Models;

namespace Consolidator.Managed.Core.Commands.Abstractions;

public enum CommandScope
{
    Coordinator,
    FocusedBank,
    ConnectedInstances
}

public interface IInstanceCommand<TResult>
{
    CommandScope Scope { get; }
}

public sealed class InstanceCommandContext
{
    public InstanceCommandContext(
        InstanceId instanceId,
        ManagedState state)
    {
        ArgumentNullException.ThrowIfNull(state);

        InstanceId = instanceId;
        State = state;
    }

    public InstanceId InstanceId { get; }

    public ManagedState State { get; }
}





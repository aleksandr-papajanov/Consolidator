using Consolidator.Managed.Core.Commands;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.Services.PerInstance;
using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class SetInstanceSoloCommandHandler
    : InstanceControlCommandHandler<SetInstanceSoloCommand>
{
    public SetInstanceSoloCommandHandler(
        InstanceRegistry instances,
        InstanceControlTargetResolver targets,
        StateHistory history)
        : base(instances, targets, history)
    {
    }

    protected override StateValue<bool> GetStateValue(ManagedInstance instance)
    {
        return instance.State.Instance.Solo;
    }
}

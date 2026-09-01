using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State;

public sealed record StateValueCreationContext(
    InstanceId InstanceId,
    StatePath Path,
    StateValueOwnership Ownership,
    StateValueEditScope Scope)
{
    public static StateValueCreationContext Instance(
        InstanceId instanceId,
        StatePath path) => new(
            instanceId,
            path,
            StateValueOwnership.InstanceOwned,
            path.Nodes[0] == StateNodeIds.Instance
                ? StateValueEditScope.Local
                : StateValueEditScope.BankGroup);

    public static StateValueCreationContext Bank(
        InstanceId instanceId,
        StatePath path,
        StateValueEditScope scope = StateValueEditScope.BankGroup) => new(
            instanceId,
            path,
            StateValueOwnership.BankOwned,
            scope);

}
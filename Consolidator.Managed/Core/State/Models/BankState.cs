using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Models;

public sealed class BankState
{
    public BankState(
        InstanceId instanceId,
        StatePath path,
        StateValueFactory values,
        BankId id,
        IStateValueObserver<GroupId?> groupObserver)
    {
        ArgumentNullException.ThrowIfNull(values);
        ArgumentNullException.ThrowIfNull(groupObserver);

        Id = id;
        Group = values.CreateBankValue(
            instanceId,
            path.Append(StateNodeIds.Group),
            (GroupId?)null,
            StateValueEditMode.CopyValue,
            scope: StateValueEditScope.Local,
            observers: [groupObserver]);
    }

    public BankId Id { get; }

    public StateValue<GroupId?> Group { get; }

}








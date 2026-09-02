using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models.Instance;

public sealed record BankState(
    BankId Id,
    StateValue<GroupId?> Group);

public sealed record InstanceState(
    InstanceId InstanceId,
    StateValue<string> Label,
    StateValue<bool> Mute,
    StateValue<bool> Solo,
    StateValue<bool> Bypass,
    BankState[] Banks);
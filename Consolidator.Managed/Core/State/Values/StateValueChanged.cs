using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Values;

public sealed record StateValueChanged(
    InstanceId InstanceId,
    StatePath Path,
    StateValueOwnership Ownership,
    object? PreviousValue,
    object? CurrentValue,
    bool IsValueChange = true);


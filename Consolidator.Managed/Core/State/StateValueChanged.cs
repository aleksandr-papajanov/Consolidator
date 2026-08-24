using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State;

public sealed record StateValueChanged(
    InstanceId InstanceId,
    StatePath Path,
    StateValueOwnership Ownership,
    object? PreviousValue,
    object? CurrentValue);


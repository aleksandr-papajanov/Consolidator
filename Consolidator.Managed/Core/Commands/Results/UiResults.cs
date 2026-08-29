using Consolidator.Managed.Core.State;
using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.Commands.Results;

public sealed record UiInitializationResult(
    ulong InstanceId,
    UiSnapshotContext SnapshotContext);

public sealed record TargetStateSnapshotResult(
    ulong InstanceId,
    int BankId,
    UiSnapshotContext SnapshotContext,
    IReadOnlyList<TargetStateValue> Values);

public sealed record TargetStateValue(
    StatePath Path,
    object? Value,
    FloatRange? PhysicalRange,
    FloatRange? EffectiveRange);

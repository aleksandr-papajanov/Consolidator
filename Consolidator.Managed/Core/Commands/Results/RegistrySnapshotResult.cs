namespace Consolidator.Managed.Core.Commands.Results;

public sealed record RegistrySnapshotResult(
    IReadOnlyList<RegistryInstanceSnapshot> Instances);

public sealed record RegistryInstanceSnapshot(
    ulong InstanceId,
    string Label,
    ulong? FocusedBankId,
    IReadOnlyList<RegistryBankSnapshot> Banks);

public sealed record RegistryBankSnapshot(
    int BankId,
    uint? GroupId);

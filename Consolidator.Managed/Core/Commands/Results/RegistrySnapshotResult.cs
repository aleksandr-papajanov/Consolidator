namespace Consolidator.Managed.Core.Commands.Results;

public sealed record RegistrySnapshotResult(
    ulong Revision,
    IReadOnlyList<RegistryInstanceSnapshot> Instances,
    IReadOnlyList<RegistryGroupSnapshot> Groups);

public sealed record RegistryInstanceSnapshot(
    ulong InstanceId,
    string Label,
    IReadOnlyList<RegistryBankSnapshot> Banks);

public sealed record RegistryBankSnapshot(
    int BankId,
    uint? GroupId);

public sealed record RegistryGroupSnapshot(
    uint GroupId,
    IReadOnlyList<RegistryGroupMemberSnapshot> Members);

public sealed record RegistryGroupMemberSnapshot(
    ulong InstanceId,
    int BankId);

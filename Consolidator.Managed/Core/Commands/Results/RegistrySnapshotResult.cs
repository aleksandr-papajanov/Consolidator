using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.Commands.Results;

public sealed record RegistrySnapshotResult(
    ulong Revision,
    IReadOnlyList<RegistryInstanceSnapshot> Instances,
    IReadOnlyList<RegistryGroupSnapshot> Groups,
    IReadOnlyList<RegistryProcessorMarkerSnapshot> ProcessorMarkers);

public sealed record RegistryProcessorMarkerSnapshot(
    ulong InstanceId,
    ProcessorId ProcessorId,
    bool Active);

public sealed record RegistryInstanceSnapshot(
    ulong InstanceId,
    string Label,
    bool Mute,
    bool Solo,
    bool Bypass,
    IReadOnlyList<ProcessorStatus> Processors,
    IReadOnlyList<RegistryBankSnapshot> Banks);

public sealed record RegistryBankSnapshot(
    int BankId,
    uint? GroupId,
    bool EffectActive);

public sealed record RegistryGroupSnapshot(
    uint GroupId,
    IReadOnlyList<RegistryGroupMemberSnapshot> Members);

public sealed record RegistryGroupMemberSnapshot(
    ulong InstanceId,
    int BankId);

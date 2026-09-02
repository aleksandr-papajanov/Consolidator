namespace Consolidator.Managed.Core.State.Definitions;

public sealed record ProcessorStatus(
    ProcessorId ProcessorId,
    bool EffectActive,
    bool Bypassed);

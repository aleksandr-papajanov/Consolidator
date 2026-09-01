namespace Consolidator.Managed.Core.State;

public sealed record ProcessorStatus(
    ProcessorId ProcessorId,
    bool EffectActive,
    bool Bypassed);

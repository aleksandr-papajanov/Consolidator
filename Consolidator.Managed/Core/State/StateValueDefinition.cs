namespace Consolidator.Managed.Core.State;

public sealed record StateValueDefinition<TValue>(
    TValue DefaultValue,
    FloatRange? PhysicalRange = null,
    bool RegisterInHistory = true);

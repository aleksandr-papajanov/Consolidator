using Consolidator.Managed.State;

namespace Consolidator.Managed.Core.State.Models.Filters;

public abstract record FilterState(
    Consolidator.Managed.Core.Settings.FilterDefinition Definition,
    StateValue<float> GainDb,
    StateValue<bool> Bypass);

public sealed record GainFilterState(
    Consolidator.Managed.Core.Settings.FilterDefinition Definition,
    StateValue<float> GainDb,
    StateValue<bool> Bypass)
    : FilterState(Definition, GainDb, Bypass);

public sealed record FixedQFilterState(
    Consolidator.Managed.Core.Settings.FilterDefinition Definition,
    StateValue<float> GainDb,
    StateValue<bool> Bypass,
    StateValue<float> FrequencyHz)
    : FilterState(Definition, GainDb, Bypass);

public sealed record BellFilterState(
    Consolidator.Managed.Core.Settings.FilterDefinition Definition,
    StateValue<float> GainDb,
    StateValue<bool> Bypass,
    StateValue<float> FrequencyHz,
    StateValue<float> Q)
    : FilterState(Definition, GainDb, Bypass);
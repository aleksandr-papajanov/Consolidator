using Consolidator.Managed.Core.Dsp;

namespace Consolidator.Managed.Approximation;

internal sealed record ApproximatedFilter(
    DspFilterType Type,
    float FrequencyHz,
    float GainDb,
    float Q);

internal sealed record ApproximationResult(
    IReadOnlyList<ApproximatedFilter> Filters,
    double Loss);

using Consolidator.Managed.Core.Dsp;

namespace Consolidator.Managed.Analysis;

internal readonly record struct EqualizerFilterSnapshot(
    DspFilterType Type,
    double FrequencyHz,
    double GainDb,
    double Q,
    bool Active);

internal sealed record EqualizerCurve(
    IReadOnlyList<double> Frequencies,
    IReadOnlyList<double> Decibels,
    double SampleRate);

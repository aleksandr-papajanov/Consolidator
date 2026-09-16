using Consolidator.Managed.Analysis;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Settings;
using NLoptNet;

namespace Consolidator.Managed.Approximation;

internal sealed class Approximator
{
    private const double LargeErrorWeight = 8.0;

    public ApproximationResult Approximate(
        EqualizerCurve target,
        OptimizationOptions options)
    {
        ArgumentNullException.ThrowIfNull(target);
        ArgumentNullException.ThrowIfNull(options);
        var parameters = CreateParameters();
        var values = parameters
            .Select(parameter => parameter.Normalize(parameter.DefaultValue))
            .ToArray();
        using var optimizer = new NLoptSolver(
            NLoptAlgorithm.LN_COBYLA,
            (uint)values.Length,
            options.FunctionTolerance,
            options.MaximumEvaluations);
        optimizer.SetLowerBounds(Enumerable.Repeat(0.0, values.Length).ToArray());
        optimizer.SetUpperBounds(Enumerable.Repeat(1.0, values.Length).ToArray());
        optimizer.SetMinObjective(candidate => Evaluate(target, parameters, candidate));
        var status = optimizer.Optimize(values, out var loss);
        if ((int)status < 0 && status is not NloptResult.ROUNDOFF_LIMITED)
        {
            throw new InvalidOperationException(
                $"Equalizer approximation failed with NLopt status {status}.");
        }

        var filters = Decode(parameters, values);
        var finalLoss = loss ?? Evaluate(target, parameters, values);
        if (!double.IsFinite(finalLoss))
        {
            throw new InvalidOperationException("Equalizer approximation failed.");
        }

        return new ApproximationResult(filters, finalLoss);
    }

    private static double Evaluate(
        EqualizerCurve target,
        IReadOnlyList<SolverParameter> parameters,
        IReadOnlyList<double> values)
    {
        var filters = CreateSnapshots(parameters, values);
        var predicted = EqualizerResponseAnalyzer.CalculateAtFrequencies(
            filters,
            target.Frequencies,
            target.SampleRate);
        var loss = 0.0;
        for (var index = 0; index < predicted.Decibels.Count; index++)
        {
            var error = predicted.Decibels[index] - target.Decibels[index];
            var squaredError = error * error;
            loss += squaredError +
                LargeErrorWeight * squaredError * squaredError;
        }

        return loss;
    }

    private static IReadOnlyList<EqualizerFilterSnapshot> CreateSnapshots(
        IReadOnlyList<SolverParameter> parameters,
        IReadOnlyList<double> values)
    {
        var result = new EqualizerFilterSnapshot[StateValueDefinitions.EqualizerDefinitions.Count];
        var position = 0;
        for (var filterIndex = 0; filterIndex < result.Length; filterIndex++)
        {
            var definition = StateValueDefinitions.EqualizerDefinitions[filterIndex];
            var frequency = definition is FrequencyFilterDefinition
                ? ReadParameter(parameters, values, ref position)
                : 0.0;
            var q = definition is BellFilterDefinition
                ? ReadParameter(parameters, values, ref position)
                : definition is FixedQFilterDefinition fixedQ ? fixedQ.FixedQ : 0.0;
            var gain = ReadParameter(parameters, values, ref position);
            result[filterIndex] = new EqualizerFilterSnapshot(
                TypeOf(definition),
                frequency,
                gain,
                q,
                true);
        }

        return result;
    }

    private static double ReadParameter(
        IReadOnlyList<SolverParameter> parameters,
        IReadOnlyList<double> values,
        ref int position)
    {
        var value = parameters[position].Denormalize(values[position]);
        position++;
        return value;
    }

    private static IReadOnlyList<ApproximatedFilter> Decode(
        IReadOnlyList<SolverParameter> parameters,
        IReadOnlyList<double> values)
    {
        var snapshots = CreateSnapshots(parameters, values);
        return snapshots
            .Select(filter => new ApproximatedFilter(
                filter.Type,
                (float)filter.FrequencyHz,
                (float)filter.GainDb,
                (float)filter.Q))
            .ToArray();
    }

    private static IReadOnlyList<SolverParameter> CreateParameters()
    {
        var parameters = new List<SolverParameter>();
        foreach (var definition in StateValueDefinitions.EqualizerDefinitions)
        {
            if (definition is FrequencyFilterDefinition frequencyDefinition)
            {
                parameters.Add(SolverParameter.From(
                    frequencyDefinition.Frequency,
                    useLogarithmicScale: true));
            }
            if (definition is BellFilterDefinition bellDefinition)
            {
                parameters.Add(SolverParameter.From(bellDefinition.Q));
            }
            parameters.Add(SolverParameter.From(definition.Gain));
        }

        return parameters;
    }

    private static DspFilterType TypeOf(FilterDefinition definition) => definition switch
    {
        GainFilterDefinition => DspFilterType.Gain,
        TiltFilterDefinition => DspFilterType.Tilt,
        LowShelfFilterDefinition => DspFilterType.LowShelf,
        HighShelfFilterDefinition => DspFilterType.HighShelf,
        BellFilterDefinition => DspFilterType.Bell,
        _ => throw new InvalidOperationException("Unknown equalizer filter definition.")
    };

}

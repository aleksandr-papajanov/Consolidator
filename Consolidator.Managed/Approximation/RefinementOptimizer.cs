using Consolidator.Managed.Analysis;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Settings;
using NLoptNet;

namespace Consolidator.Managed.Approximation;

internal sealed class RefinementOptimizer
{
    private const double LargeErrorWeight = 8.0;

    public RefinementCandidate? Optimize(
        EqualizerCurve target,
        IReadOnlyList<int> filterIndices,
        IReadOnlyList<SolverParameter> parameters,
        IReadOnlyList<double> initialValues,
        OptimizationOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var values = initialValues.ToArray();
        var objectiveTarget = Downsample(target, options.MaximumFrequencyPoints);
        if (values.Length == 0)
        {
            var emptyLoss = EvaluateResponse(
                objectiveTarget,
                filterIndices,
                parameters,
                values);
            var emptyResponse = EvaluateResponseCurve(
                target,
                filterIndices,
                parameters,
                values);
            return new RefinementCandidate(
                filterIndices,
                parameters,
                values,
                emptyLoss,
                emptyLoss,
                emptyResponse.Decibels);
        }

        using var optimizer = new NLoptSolver(
            NLoptAlgorithm.LN_COBYLA,
            (uint)values.Length,
            options.FunctionTolerance,
            options.MaximumEvaluations);
        optimizer.SetLowerBounds(Enumerable.Repeat(0.0, values.Length).ToArray());
        optimizer.SetUpperBounds(Enumerable.Repeat(1.0, values.Length).ToArray());
        optimizer.SetMinObjective(candidate => EvaluateResponse(
            objectiveTarget,
            filterIndices,
            parameters,
            candidate));
        var status = optimizer.Optimize(values, out var loss);
        if ((int)status < 0 && status is not NloptResult.ROUNDOFF_LIMITED)
        {
            return null;
        }

        var responseLoss = loss ?? EvaluateResponse(
            target,
            filterIndices,
            parameters,
            values);
        if (!double.IsFinite(responseLoss))
        {
            return null;
        }

        var response = EvaluateResponseCurve(target, filterIndices, parameters, values);
        return new RefinementCandidate(
            filterIndices,
            parameters,
            values,
            responseLoss,
            responseLoss,
            response.Decibels);
    }

    public double EvaluateEmpty(EqualizerCurve target) =>
        EvaluateResponse(target, [], [], []);

    private static double EvaluateResponse(
        EqualizerCurve target,
        IReadOnlyList<int> filterIndices,
        IReadOnlyList<SolverParameter> parameters,
        IReadOnlyList<double> values)
    {
        var predicted = EvaluateResponseCurve(target, filterIndices, parameters, values);
        var loss = 0.0;
        for (var index = 0; index < predicted.Decibels.Count; index++)
        {
            var error = predicted.Decibels[index] - target.Decibels[index];
            var squaredError = error * error;
            loss += FrequencyWeight(target.Frequencies, index) *
                (squaredError + LargeErrorWeight * squaredError * squaredError);
        }

        return target.Frequencies.Count == 0
            ? 0.0
            : loss / target.Frequencies.Count;
    }

    private static EqualizerCurve Downsample(
        EqualizerCurve target,
        int maximumFrequencyPoints)
    {
        if (maximumFrequencyPoints <= 0 ||
            maximumFrequencyPoints >= target.Frequencies.Count ||
            target.Frequencies.Count <= 2)
        {
            return target;
        }

        var indices = Enumerable.Range(0, maximumFrequencyPoints)
            .Select(point => (int)Math.Round(
                point * (target.Frequencies.Count - 1.0) /
                    (maximumFrequencyPoints - 1)))
            .Distinct()
            .ToArray();
        return new EqualizerCurve(
            indices.Select(index => target.Frequencies[index]).ToArray(),
            indices.Select(index => target.Decibels[index]).ToArray(),
            target.SampleRate);
    }

    private static EqualizerCurve EvaluateResponseCurve(
        EqualizerCurve target,
        IReadOnlyList<int> filterIndices,
        IReadOnlyList<SolverParameter> parameters,
        IReadOnlyList<double> values)
    {
        var filters = CreateCandidateSnapshots(filterIndices, parameters, values);
        return EqualizerResponseAnalyzer.CalculateAtFrequencies(
            filters,
            target.Frequencies,
            target.SampleRate);
    }

    private static double FrequencyWeight(
        IReadOnlyList<double> frequencies,
        int index)
    {
        if (frequencies.Count < 2 || frequencies.Any(frequency => frequency <= 0.0))
        {
            return 1.0;
        }

        var current = Math.Log(frequencies[index]);
        if (index == 0)
        {
            return Math.Abs(Math.Log(frequencies[1]) - current);
        }

        if (index == frequencies.Count - 1)
        {
            return Math.Abs(current - Math.Log(frequencies[index - 1]));
        }

        return Math.Abs(Math.Log(frequencies[index + 1]) -
            Math.Log(frequencies[index - 1])) / 2.0;
    }

    private static IReadOnlyList<EqualizerFilterSnapshot> CreateCandidateSnapshots(
        IReadOnlyList<int> filterIndices,
        IReadOnlyList<SolverParameter> parameters,
        IReadOnlyList<double> values)
    {
        var result = StateValueDefinitions.EqualizerDefinitions
            .Select(definition => new EqualizerFilterSnapshot(
                TypeOf(definition),
                definition is FrequencyFilterDefinition frequency
                    ? frequency.Frequency.DefaultValue
                    : 0.0,
                0.0,
                definition is BellFilterDefinition bell
                    ? bell.Q.DefaultValue
                    : definition is FixedQFilterDefinition fixedQ
                        ? fixedQ.FixedQ
                        : 0.0,
                false))
            .ToArray();
        var position = 0;
        foreach (var filterIndex in filterIndices)
        {
            var definition = StateValueDefinitions.EqualizerDefinitions[filterIndex];
            var frequency = definition is FrequencyFilterDefinition
                ? ReadParameter(parameters, values, ref position)
                : 0.0;
            var q = definition is BellFilterDefinition
                ? ReadParameter(parameters, values, ref position)
                : definition is FixedQFilterDefinition fixedQ
                    ? fixedQ.FixedQ
                    : 0.0;
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

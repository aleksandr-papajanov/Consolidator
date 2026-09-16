using Consolidator.Managed.Analysis;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Settings;

namespace Consolidator.Managed.Approximation;

internal sealed class EqualizerRefinement
    : IBeamSearchPolicy<EqualizerCurve, RefinementCandidate, ApproximationResult>
{
    private const double MinimumScoreImprovement = 0.0001;
    private const double SimplificationTolerance = 0.0001;
    private const double ComplexityPenalty = 0.002;
    private const double WeakFilterPenalty = 0.01;
    private const double RedundancyPenalty = 0.02;
    private const double ExtremeParameterPenalty = 0.01;
    private const double WeakGainThreshold = 0.75;
    private const double ResponseEquivalenceTolerance = 0.003;
    private const double ExactResponseLoss = 1.0e-8;
    private const int FinalistCount = 2;
    private const int BeamWidthValue = 6;
    private const int MaximumNonImprovingDepthsValue = 3;

    private readonly RefinementOptimizer _optimizer;

    public EqualizerRefinement(RefinementOptimizer optimizer)
    {
        _optimizer = optimizer;
    }

    public int BeamWidth => BeamWidthValue;

    public int MaximumDepth => StateValueDefinitions.EqualizerDefinitions.Count;

    public int MaximumNonImprovingDepths => MaximumNonImprovingDepthsValue;

    public ApproximationResult Refine(EqualizerCurve input)
    {
        return new BeamSearch<EqualizerCurve, RefinementCandidate, ApproximationResult>(this)
            .Search(input);
    }

    public RefinementCandidate CreateInitialState(EqualizerCurve input) =>
        CreateEmptyCandidate(input);

    public IReadOnlyList<RefinementCandidate> Expand(
        EqualizerCurve input,
        RefinementCandidate state)
    {
        var expansions = new List<RefinementCandidate>();
        var seenStructures = new HashSet<string>();
        for (var filterIndex = 0;
             filterIndex < StateValueDefinitions.EqualizerDefinitions.Count;
             filterIndex++)
        {
            if (state.FilterIndices.Contains(filterIndex))
            {
                continue;
            }

            var filterIndices = state.FilterIndices
                .Append(filterIndex)
                .OrderBy(index => index)
                .ToArray();
            if (!seenStructures.Add(string.Join(',', filterIndices)))
            {
                continue;
            }

            var candidate = _optimizer.Optimize(
                input,
                filterIndices,
                CreateParameters(filterIndices),
                CreateInitialValues(filterIndices, CreateResidualTarget(input, state)),
                OptimizationOptions.Fast);
            if (candidate is not null)
            {
                expansions.Add(candidate with
                {
                    Score = ScoreCandidate(input, candidate)
                });
            }
        }

        return expansions;
    }

    public BeamSearchEvaluation<RefinementCandidate> Evaluate(
        EqualizerCurve input,
        RefinementCandidate state)
    {
        var evaluated = state with { Score = ScoreCandidate(input, state) };
        return new BeamSearchEvaluation<RefinementCandidate>(evaluated, evaluated.Score);
    }

    public IReadOnlyList<RefinementCandidate> SelectDiverse(
        IReadOnlyList<BeamSearchEvaluation<RefinementCandidate>> candidates)
    {
        var selected = new List<RefinementCandidate>(BeamWidth);
        foreach (var candidate in candidates.OrderBy(candidate => candidate.Score))
        {
            if (selected.All(existing => ResponseDistance(
                    existing.Response,
                    candidate.State.Response) >= ResponseEquivalenceTolerance))
            {
                selected.Add(candidate.State);
            }

            if (selected.Count == BeamWidth)
            {
                break;
            }
        }

        return selected;
    }

    public bool IsMeaningfulImprovement(double candidateScore, double bestScore) =>
        candidateScore < bestScore - MinimumScoreImprovement;

    public bool IsTerminal(RefinementCandidate state) =>
        state.ResponseLoss <= ExactResponseLoss;

    public ApproximationResult Complete(
        EqualizerCurve input,
        RefinementCandidate bestState,
        IReadOnlyList<RefinementCandidate> beam)
    {
        var finalists = beam
            .Append(bestState)
            .OrderBy(candidate => candidate.Score)
            .Take(FinalistCount)
            .Select(candidate => _optimizer.Optimize(
                input,
                candidate.FilterIndices,
                candidate.Parameters,
                candidate.Values,
                OptimizationOptions.Balanced))
            .Where(candidate => candidate is not null)
            .Select(candidate => candidate! with
            {
                Score = ScoreCandidate(input, candidate)
            })
            .Append(bestState)
            .Append(CreateInitialState(input))
            .OrderBy(candidate => candidate.Score)
            .ToList();
        var simplified = Simplify(input, finalists[0]);
        return new ApproximationResult(DecodeCandidate(simplified), simplified.ResponseLoss);
    }

    private static double ResponseDistance(
        IReadOnlyList<double> left,
        IReadOnlyList<double> right)
    {
        if (left.Count != right.Count || left.Count == 0)
        {
            return double.PositiveInfinity;
        }

        var sum = 0.0;
        for (var index = 0; index < left.Count; index++)
        {
            var difference = left[index] - right[index];
            sum += difference * difference;
        }

        return Math.Sqrt(sum / left.Count);
    }

    private RefinementCandidate CreateEmptyCandidate(EqualizerCurve target)
    {
        var responseLoss = _optimizer.EvaluateEmpty(target);
        return new RefinementCandidate([], [], [], responseLoss, responseLoss, []);
    }

    private double ScoreCandidate(
        EqualizerCurve target,
        RefinementCandidate candidate)
    {
        var score = candidate.ResponseLoss +
            ComplexityPenalty * candidate.FilterIndices.Count;
        var position = 0;
        foreach (var filterIndex in candidate.FilterIndices)
        {
            var definition = StateValueDefinitions.EqualizerDefinitions[filterIndex];
            if (definition is FrequencyFilterDefinition)
            {
                position++;
                if (definition is BellFilterDefinition)
                {
                    position++;
                }
            }

            var gain = candidate.Parameters[position].Denormalize(candidate.Values[position]);
            position++;
            if (Math.Abs(gain) < WeakGainThreshold)
            {
                score += WeakFilterPenalty;
            }

            var normalizedDistance = Math.Min(
                candidate.Values[position - 1],
                1.0 - candidate.Values[position - 1]);
            if (normalizedDistance < 0.03)
            {
                score += ExtremeParameterPenalty;
            }
        }

        var snapshots = CreateCandidateSnapshots(candidate);
        var contributions = candidate.FilterIndices
            .Select(index => EqualizerResponseAnalyzer.CalculateAtFrequencies(
                [snapshots[index]],
                target.Frequencies,
                target.SampleRate).Decibels)
            .ToArray();
        for (var left = 0; left < contributions.Length; left++)
        {
            for (var right = left + 1; right < contributions.Length; right++)
            {
                var overlap = ShapeSimilarity(contributions[left], contributions[right]);
                if (overlap < 0.9)
                {
                    continue;
                }

                score += RedundancyPenalty * overlap;
                var combinedNorm = Norm(contributions[left].Zip(
                    contributions[right], (first, second) => first + second));
                var separateNorm = Norm(contributions[left]) + Norm(contributions[right]);
                if (separateNorm > 0.0 && combinedNorm / separateNorm < 0.35)
                {
                    score += RedundancyPenalty * 2.0;
                }
            }
        }

        return score;
    }

    private RefinementCandidate Simplify(
        EqualizerCurve target,
        RefinementCandidate candidate)
    {
        var current = candidate;
        var changed = true;
        while (changed && current.FilterIndices.Count > 0)
        {
            changed = false;
            RefinementCandidate? bestRemoval = null;
            for (var removePosition = 0;
                 removePosition < current.FilterIndices.Count;
                 removePosition++)
            {
                var filterIndices = current.FilterIndices
                    .Where((_, position) => position != removePosition)
                    .ToArray();
                var removal = _optimizer.Optimize(
                    target,
                    filterIndices,
                    CreateParameters(filterIndices),
                    CreateInitialValues(filterIndices, target),
                    OptimizationOptions.Fast);
                if (removal is not null)
                {
                    removal = removal with
                    {
                        Score = ScoreCandidate(target, removal)
                    };
                }
                if (removal is not null &&
                    removal.Score <= current.Score + SimplificationTolerance &&
                    (bestRemoval is null || removal.Score < bestRemoval.Score))
                {
                    bestRemoval = removal;
                }
            }

            if (bestRemoval is not null)
            {
                current = bestRemoval;
                changed = true;
            }
        }

        return current;
    }

    private EqualizerCurve CreateResidualTarget(
        EqualizerCurve target,
        RefinementCandidate parent)
    {
        if (parent.Response.Count == 0)
        {
            return target;
        }

        var residual = target.Decibels
            .Select((value, index) => value - parent.Response[index])
            .ToArray();
        return new EqualizerCurve(target.Frequencies, residual, target.SampleRate);
    }

    private static double[] CreateInitialValues(
        IReadOnlyList<int> filterIndices,
        EqualizerCurve target)
    {
        var peakIndex = 0;
        for (var index = 1; index < target.Decibels.Count; index++)
        {
            if (Math.Abs(target.Decibels[index]) > Math.Abs(target.Decibels[peakIndex]))
            {
                peakIndex = index;
            }
        }

        var peakFrequency = target.Frequencies.Count == 0 ? 1000.0 : target.Frequencies[peakIndex];
        var peakGain = target.Decibels.Count == 0 ? 0.0 : target.Decibels[peakIndex];
        var parameters = CreateParameters(filterIndices);
        var values = parameters.Select(parameter => parameter.Normalize(parameter.DefaultValue)).ToArray();
        var position = 0;
        foreach (var filterIndex in filterIndices)
        {
            var definition = StateValueDefinitions.EqualizerDefinitions[filterIndex];
            if (definition is FrequencyFilterDefinition frequencyDefinition)
            {
                values[position] = parameters[position].Normalize(Math.Clamp(
                    peakFrequency,
                    frequencyDefinition.Frequency.Range.Minimum,
                    frequencyDefinition.Frequency.Range.Maximum));
                position++;
            }
            if (definition is BellFilterDefinition bellDefinition)
            {
                values[position] = parameters[position].Normalize(Math.Clamp(
                    1.0,
                    bellDefinition.Q.Range.Minimum,
                    bellDefinition.Q.Range.Maximum));
                position++;
            }
            values[position] = parameters[position].Normalize(Math.Clamp(
                peakGain,
                definition.Gain.Range.Minimum,
                definition.Gain.Range.Maximum));
            position++;
        }
        return values;
    }

    private static IReadOnlyList<SolverParameter> CreateParameters(
        IReadOnlyList<int> filterIndices)
    {
        var parameters = new List<SolverParameter>();
        foreach (var filterIndex in filterIndices)
        {
            var definition = StateValueDefinitions.EqualizerDefinitions[filterIndex];
            if (definition is FrequencyFilterDefinition frequencyDefinition)
            {
                parameters.Add(SolverParameter.From(frequencyDefinition.Frequency, true));
            }
            if (definition is BellFilterDefinition bellDefinition)
            {
                parameters.Add(SolverParameter.From(bellDefinition.Q));
            }
            parameters.Add(SolverParameter.From(definition.Gain));
        }
        return parameters;
    }

    private static IReadOnlyList<EqualizerFilterSnapshot> CreateCandidateSnapshots(
        RefinementCandidate candidate)
    {
        var result = StateValueDefinitions.EqualizerDefinitions
            .Select(definition => new EqualizerFilterSnapshot(
                TypeOf(definition),
                definition is FrequencyFilterDefinition frequency ? frequency.Frequency.DefaultValue : 0.0,
                0.0,
                definition is BellFilterDefinition bell ? bell.Q.DefaultValue :
                    definition is FixedQFilterDefinition fixedQ ? fixedQ.FixedQ : 0.0,
                false))
            .ToArray();
        var position = 0;
        foreach (var filterIndex in candidate.FilterIndices)
        {
            var definition = StateValueDefinitions.EqualizerDefinitions[filterIndex];
            var frequency = definition is FrequencyFilterDefinition
                ? candidate.Parameters[position].Denormalize(candidate.Values[position++])
                : 0.0;
            var q = definition is BellFilterDefinition
                ? candidate.Parameters[position].Denormalize(candidate.Values[position++])
                : definition is FixedQFilterDefinition fixedQ
                    ? fixedQ.FixedQ
                    : 0.0;
            var gain = candidate.Parameters[position].Denormalize(candidate.Values[position++]);
            result[filterIndex] = new EqualizerFilterSnapshot(
                TypeOf(definition), frequency, gain, q, true);
        }
        return result;
    }

    private static IReadOnlyList<ApproximatedFilter> DecodeCandidate(
        RefinementCandidate candidate) =>
        CreateCandidateSnapshots(candidate)
            .Select(filter => new ApproximatedFilter(
                filter.Type,
                (float)filter.FrequencyHz,
                (float)filter.GainDb,
                (float)filter.Q))
            .ToArray();

    private static double ShapeSimilarity(
        IReadOnlyList<double> left,
        IReadOnlyList<double> right)
    {
        var leftMean = left.Count == 0 ? 0.0 : left.Average();
        var rightMean = right.Count == 0 ? 0.0 : right.Average();
        var centeredLeft = left.Select(value => value - leftMean).ToArray();
        var centeredRight = right.Select(value => value - rightMean).ToArray();
        var denominator = Norm(centeredLeft) * Norm(centeredRight);
        return denominator <= double.Epsilon
            ? 0.0
            : Math.Abs(centeredLeft.Zip(centeredRight,
                (first, second) => first * second).Sum() / denominator);
    }

    private static double Norm(IEnumerable<double> values) =>
        Math.Sqrt(values.Sum(value => value * value));

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

namespace Consolidator.Managed.Approximation;

internal readonly record struct BeamSearchEvaluation<TState>(
    TState State,
    double Score);

internal interface IBeamSearchPolicy<TInput, TState, TResult>
{
    int BeamWidth { get; }

    int MaximumDepth { get; }

    int MaximumNonImprovingDepths { get; }

    TState CreateInitialState(TInput input);

    IReadOnlyList<TState> Expand(TInput input, TState state);

    BeamSearchEvaluation<TState> Evaluate(TInput input, TState state);

    IReadOnlyList<TState> SelectDiverse(
        IReadOnlyList<BeamSearchEvaluation<TState>> candidates);

    bool IsMeaningfulImprovement(double candidateScore, double bestScore);

    bool IsTerminal(TState state);

    TResult Complete(
        TInput input,
        TState bestState,
        IReadOnlyList<TState> beam);
}

internal sealed class BeamSearch<TInput, TState, TResult>
{
    private readonly IBeamSearchPolicy<TInput, TState, TResult> _policy;

    public BeamSearch(IBeamSearchPolicy<TInput, TState, TResult> policy)
    {
        _policy = policy;
    }

    public TResult Search(TInput input)
    {
        var bestState = _policy.CreateInitialState(input);
        var beam = new List<TState> { bestState };
        var bestScore = _policy.Evaluate(input, bestState).Score;
        var nonImprovingDepths = 0;

        for (var depth = 1; depth <= _policy.MaximumDepth; depth++)
        {
            var candidates = beam
                .SelectMany(state => _policy.Expand(input, state))
                .Select(state => _policy.Evaluate(input, state))
                .ToArray();
            if (candidates.Length == 0)
            {
                break;
            }

            var nextBeam = _policy.SelectDiverse(candidates);
            if (nextBeam.Count == 0)
            {
                break;
            }

            var depthBest = candidates
                .OrderBy(candidate => candidate.Score)
                .First();
            if (_policy.IsMeaningfulImprovement(depthBest.Score, bestScore))
            {
                bestState = depthBest.State;
                bestScore = depthBest.Score;
                nonImprovingDepths = 0;
            }
            else
            {
                nonImprovingDepths++;
                if (nonImprovingDepths >= _policy.MaximumNonImprovingDepths)
                {
                    break;
                }
            }

            beam = nextBeam.ToList();
            if (_policy.IsTerminal(bestState))
            {
                break;
            }
        }

        return _policy.Complete(input, bestState, beam);
    }
}

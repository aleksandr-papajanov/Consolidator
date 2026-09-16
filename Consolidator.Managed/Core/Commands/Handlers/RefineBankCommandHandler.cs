using Consolidator.Managed.Analysis;
using Consolidator.Managed.Approximation;
using Consolidator.Managed.Core.Commands.Abstractions;
using Consolidator.Managed.Core.Commands.Definitions;
using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.Services.PerInstance;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Commands.Handlers;

internal sealed class RefineBankCommandHandler
    : CommandHandler<RefineBankCommand, StateWriteStatus>
{
    private const double MinimumUsefulGainDb = 0.75;

    private readonly InstanceRegistry _instances;
    private readonly InstanceControlTargetResolver _targets;
    private readonly FftAnalyzer _analyzer;
    private readonly EqualizerRefinement _refiner;
    private readonly StateHistory _history;

    public RefineBankCommandHandler(
        InstanceRegistry instances,
        InstanceControlTargetResolver targets,
        FftAnalyzer analyzer,
        EqualizerRefinement refiner,
        StateHistory history)
    {
        _instances = instances;
        _targets = targets;
        _analyzer = analyzer;
        _refiner = refiner;
        _history = history;
    }

    public override async ValueTask<StateWriteStatus> HandleAsync(
        RefineBankCommand command,
        InstanceCommandContext context,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (command.TargetInstanceId is not { } targetInstanceId ||
            !_targets.TryResolveBanks(
                command.TargetScope,
                context,
                targetInstanceId,
                command.BankIndex,
                out var addresses))
        {
            return StateWriteStatus.Rejected;
        }

        var snapshots = new List<ApproximationTarget>(addresses.Count);
        foreach (var address in addresses)
        {
            var instance = _instances.FindInstance(address.InstanceId);
            if (instance is null ||
                (uint)address.BankIndex >= instance.State.Instance.Banks.Length)
            {
                return StateWriteStatus.Rejected;
            }

            var source = instance.State.Instance.Banks[address.BankIndex];
            var filters = CaptureFilters(source.Equalizer.Filters);
            if (!filters.Any(filter => filter.Active &&
                    Math.Abs(filter.GainDb) > ActivityObserver.ActivityEpsilon))
            {
                return StateWriteStatus.Rejected;
            }

            snapshots.Add(new ApproximationTarget(instance, address.BankIndex, filters));
        }

        var results = await Task.WhenAll(snapshots.Select(snapshot => Task.Run(
            () =>
            {
                cancellationToken.ThrowIfCancellationRequested();
                var curve = _analyzer.AnalyzeEqualizerResponse(
                    snapshot.Instance.InstanceId,
                    snapshot.Filters);
                return _refiner.Refine(curve);
            },
            cancellationToken)));
        if (results.Any(result => result is null))
        {
            return StateWriteStatus.Rejected;
        }

        cancellationToken.ThrowIfCancellationRequested();
        using var transaction = _history.BeginTransaction();
        for (var index = 0; index < snapshots.Count; index++)
        {
            ApplyResult(
                snapshots[index].Instance.State.Instance.Banks[
                    snapshots[index].BankIndex],
                results[index]!,
                transaction);
        }

        _history.AdvanceHistoryPoint();
        transaction.Commit();
        return StateWriteStatus.Applied;
    }

    private static EqualizerFilterSnapshot[] CaptureFilters(
        IReadOnlyList<FilterState> filters)
    {
        return filters.Select(filter => filter switch
        {
            GainFilterState gain => new EqualizerFilterSnapshot(
                DspFilterType.Gain, 0, gain.GainDb.Value, 0, !gain.Bypass.Value),
            FixedQFilterState fixedQ => new EqualizerFilterSnapshot(
                TypeOf(fixedQ), fixedQ.FrequencyHz.Value, fixedQ.GainDb.Value,
                ((FixedQFilterDefinition)fixedQ.Definition).FixedQ, !fixedQ.Bypass.Value),
            BellFilterState bell => new EqualizerFilterSnapshot(
                DspFilterType.Bell, bell.FrequencyHz.Value, bell.GainDb.Value,
                bell.Q.Value, !bell.Bypass.Value),
            _ => throw new InvalidOperationException("Unknown equalizer filter state.")
        }).ToArray();
    }

    private static DspFilterType TypeOf(FixedQFilterState filter) => filter.Definition switch
    {
        TiltFilterDefinition => DspFilterType.Tilt,
        LowShelfFilterDefinition => DspFilterType.LowShelf,
        HighShelfFilterDefinition => DspFilterType.HighShelf,
        _ => throw new InvalidOperationException("Unknown fixed-Q filter state.")
    };

    private static DspFilterType TypeOf(FilterState filter) => filter switch
    {
        GainFilterState => DspFilterType.Gain,
        FixedQFilterState fixedQ => TypeOf(fixedQ),
        BellFilterState => DspFilterType.Bell,
        _ => throw new InvalidOperationException("Unknown equalizer filter state.")
    };

    private static void ApplyResult(
        BankState destination,
        ApproximationResult result,
        StateHistoryTransaction transaction)
    {
        if (destination.Equalizer.Filters.Length != result.Filters.Count)
        {
            throw new InvalidOperationException("Approximation result size does not match Bank0.");
        }

        destination.Bypass.Prepare(false, transaction);
        for (var index = 0; index < destination.Equalizer.Filters.Length; index++)
        {
            var state = destination.Equalizer.Filters[index];
            var fitted = result.Filters[index];
            if (fitted.Type != TypeOf(state))
            {
                throw new InvalidOperationException(
                    $"Approximation filter type mismatch at index {index}.");
            }
            state.Bypass.Prepare(
                Math.Abs(fitted.GainDb) < MinimumUsefulGainDb,
                transaction);
            state.GainDb.Prepare(fitted.GainDb, transaction);
            if (state is FixedQFilterState fixedQ)
            {
                fixedQ.FrequencyHz.Prepare(fitted.FrequencyHz, transaction);
            }
            if (state is BellFilterState bell)
            {
                bell.FrequencyHz.Prepare(fitted.FrequencyHz, transaction);
                bell.Q.Prepare(fitted.Q, transaction);
            }
        }
    }

    private sealed record ApproximationTarget(
        ManagedInstance Instance,
        int BankIndex,
        IReadOnlyList<EqualizerFilterSnapshot> Filters);
}

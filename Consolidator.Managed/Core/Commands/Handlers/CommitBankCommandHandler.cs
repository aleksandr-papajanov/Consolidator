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

internal sealed class CommitBankCommandHandler
    : CommandHandler<CommitBankCommand, StateWriteStatus>
{
    private const BankId DestinationBankId = BankId.Bank0;

    private readonly InstanceRegistry _instances;
    private readonly InstanceControlTargetResolver _targets;
    private readonly FftAnalyzer _analyzer;
    private readonly Approximator _approximator;
    private readonly StateHistory _history;

    public CommitBankCommandHandler(
        InstanceRegistry instances,
        InstanceControlTargetResolver targets,
        FftAnalyzer analyzer,
        Approximator approximator,
        StateHistory history)
    {
        _instances = instances;
        _targets = targets;
        _analyzer = analyzer;
        _approximator = approximator;
        _history = history;
    }

    public override async ValueTask<StateWriteStatus> HandleAsync(
        CommitBankCommand command,
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

            var bank = instance.State.Instance.Banks[address.BankIndex];
            var filters = CaptureFilters(bank.Equalizer.Filters);
            var isAddressedBank = address.InstanceId == targetInstanceId &&
                address.BankIndex == command.BankIndex;
            if (isAddressedBank &&
                !filters.Any(filter => filter.Active &&
                    Math.Abs(filter.GainDb) > ActivityObserver.ActivityEpsilon))
            {
                return StateWriteStatus.Rejected;
            }

            if (address.BankIndex != (int)DestinationBankId)
            {
                var destination = instance.State.Instance.Banks[(int)DestinationBankId];
                filters = filters
                    .Concat(CaptureFilters(destination.Equalizer.Filters))
                    .ToArray();
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
                return _approximator.Approximate(
                    curve,
                    OptimizationOptions.Commit);
            },
            cancellationToken)));

        cancellationToken.ThrowIfCancellationRequested();
        using var transaction = _history.BeginTransaction();
        for (var index = 0; index < snapshots.Count; index++)
        {
            ApplyResult(
                snapshots[index].Instance.State.Instance.Banks[(int)DestinationBankId],
                results[index],
                transaction);
            if (snapshots[index].SourceBankIndex != (int)DestinationBankId)
            {
                ResetBank(
                    snapshots[index].Instance.State.Instance.Banks[
                        snapshots[index].SourceBankIndex],
                    transaction);
            }
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
                DspFilterType.Gain,
                0,
                gain.GainDb.Value,
                0,
                !gain.Bypass.Value),
            FixedQFilterState fixedQ => new EqualizerFilterSnapshot(
                TypeOf(fixedQ),
                fixedQ.FrequencyHz.Value,
                fixedQ.GainDb.Value,
                ((FixedQFilterDefinition)fixedQ.Definition).FixedQ,
                !fixedQ.Bypass.Value),
            BellFilterState bell => new EqualizerFilterSnapshot(
                DspFilterType.Bell,
                bell.FrequencyHz.Value,
                bell.GainDb.Value,
                bell.Q.Value,
                !bell.Bypass.Value),
            _ => throw new InvalidOperationException("Unknown equalizer filter state.")
        }).ToArray();
    }

    private static DspFilterType TypeOf(FixedQFilterState filter) =>
        filter.Definition switch
        {
            TiltFilterDefinition => DspFilterType.Tilt,
            LowShelfFilterDefinition => DspFilterType.LowShelf,
            HighShelfFilterDefinition => DspFilterType.HighShelf,
            _ => throw new InvalidOperationException("Unknown fixed-Q filter state.")
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
            state.Bypass.Prepare(false, transaction);
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

    private static DspFilterType TypeOf(FilterState filter) => filter switch
    {
        GainFilterState => DspFilterType.Gain,
        FixedQFilterState fixedQ => TypeOf(fixedQ),
        BellFilterState => DspFilterType.Bell,
        _ => throw new InvalidOperationException("Unknown equalizer filter state.")
    };

    private static void ResetBank(
        BankState bank,
        StateHistoryTransaction transaction)
    {
        bank.Bypass.PrepareResetDirect(transaction);
        foreach (var filter in bank.Equalizer.Filters)
        {
            filter.Bypass.PrepareResetDirect(transaction);
            filter.GainDb.PrepareResetDirect(transaction);
            if (filter is FixedQFilterState fixedQ)
            {
                fixedQ.FrequencyHz.PrepareResetDirect(transaction);
            }
            if (filter is BellFilterState bell)
            {
                bell.FrequencyHz.PrepareResetDirect(transaction);
                bell.Q.PrepareResetDirect(transaction);
            }
        }
    }

    private sealed record ApproximationTarget(
        ManagedInstance Instance,
        int SourceBankIndex,
        IReadOnlyList<EqualizerFilterSnapshot> Filters);
}

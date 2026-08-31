using System.Text.Json;

using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.Core.State.Models;
using Consolidator.Managed.State;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Services.Persistence;

internal sealed class InstancePersistenceService
{
    private const int CurrentSchema = 2;
    private readonly InstanceRegistry _instances;
    private readonly IOperationGate _operationGate;
    private readonly PersistenceChangePublisher _persistenceChanges;
    private readonly DspStateChangeTracker _dspChanges;

    internal InstancePersistenceService(
        InstanceRegistry instances,
        IOperationGate operationGate,
        PersistenceChangePublisher persistenceChanges,
        DspStateChangeTracker dspChanges)
    {
        _instances = instances;
        _operationGate = operationGate;
        _persistenceChanges = persistenceChanges;
        _dspChanges = dspChanges;
    }

    public byte[] CaptureCommitted(InstanceId instanceId)
    {
        using (_operationGate.Enter())
        {
            var instance = _instances.FindInstance(instanceId)
                ?? throw new InvalidOperationException(
                    "Instance is not registered.");
            return JsonSerializer.SerializeToUtf8Bytes(
                CreateSnapshot(instance.State),
                PersistenceJsonContext.Default.PersistentStateV2);
        }
    }

    public void Restore(InstanceId instanceId, ReadOnlySpan<byte> utf8Json)
    {
        var snapshot = JsonSerializer.Deserialize(
            utf8Json,
            PersistenceJsonContext.Default.PersistentStateV2)
            ?? throw new InvalidDataException("Persistence payload is empty.");
        Validate(snapshot);
        using (_operationGate.Enter())
        {
            var instance = _instances.FindInstance(instanceId)
                ?? throw new InvalidOperationException(
                    "Instance is not registered.");
            using var suppression = _persistenceChanges.Suppress(instanceId);
            Apply(instance.State, snapshot);
            _instances.PublishDspStates(_dspChanges.Drain());
        }
    }

    private static PersistentStateV2 CreateSnapshot(ManagedState state) => new(
        CurrentSchema,
        new PersistentInstance(
            state.Instance.Mute.Value,
            state.Instance.Solo.Value),
        state.Instance.Banks
            .Select(bank => new PersistentBank(bank.Group.Value?.Value))
            .ToArray(),
        new PersistentDsp(
            CreateInput(state.Dsp.InputGain),
            CreateSaturator(state.Dsp.Saturator),
            CreateCompressor(state.Dsp.Compressor),
            CreateEqualizer(state.Dsp.Equalizer, state.Dsp.EqualizerBanks),
            CreatePolish(state.Dsp.Polish),
            CreateOutput(state.Dsp.OutputGain)));

    private static PersistentInput CreateInput(InputState input) =>
        new(input.Level.Value, input.Target.Value, input.Width.Value,
            input.Leveler.Value, input.Bypass.Value);

    private static PersistentSaturator CreateSaturator(SaturatorState value) => new(
        value.Drive.Value, value.Curve.Value, value.Split.Value,
        value.OutputDb.Value, value.Bypass.Value, value.Solo.Value,
        CreateDetector(value.Detector));

    private static PersistentCompressor CreateCompressor(CompressorState value) => new(
        value.Attack.Value, value.Sustain.Value, value.Compression.Value,
        value.Character.Value, value.Parallel.Value, value.OutputDb.Value,
        value.Bypass.Value, value.Solo.Value, CreateDetector(value.Detector));

    private static PersistentPolish CreatePolish(PolishState value) =>
        new(value.Thick.Value, value.Air.Value, value.Bypass.Value, value.Solo.Value);

    private static PersistentOutput CreateOutput(OutputState value) =>
        new(value.Level.Value, value.Target.Value, value.Limiter.Value, value.Bypass.Value);

    private static PersistentDetector CreateDetector(DetectorState value) => new(
        value.Listen.Value, value.Filters.Select(CreateFilter).ToArray());

    private static PersistentEqualizer CreateEqualizer(
        EqualizerState equalizer,
        IReadOnlyList<EqualizerBankState> banks) => new(
        equalizer.Bypass.Value,
        equalizer.Solo.Value,
        banks.Select(bank => new PersistentEqualizerBank(
            bank.Bypass.Value,
            bank.Solo.Value,
            bank.Filters.Select(CreateFilter).ToArray())).ToArray());

    private static PersistentFilter CreateFilter(FilterState value) => new(
        value.FrequencyHz?.Value,
        value.Q?.Value,
        value.GainDb.Value,
        value.Bypass.Value,
        value.Solo.Value);

    private static void Apply(ManagedState state, PersistentStateV2 snapshot)
    {
        using var transaction = new StateHistoryTransaction();
        state.Instance.Mute.PrepareBaseline(snapshot.Instance.Mute, transaction);
        state.Instance.Solo.PrepareBaseline(snapshot.Instance.Solo, transaction);
        for (var index = 0; index < DspConstants.BankCount; index++)
        {
            state.Instance.Banks[index].Group.PrepareBaseline(
                snapshot.Banks[index].Group is { } group ? new GroupId(group) : null,
                transaction);
        }

        ApplyInput(state.Dsp.InputGain, snapshot.Dsp.Input, transaction);
        ApplySaturator(state.Dsp.Saturator, snapshot.Dsp.Saturator, transaction);
        ApplyCompressor(state.Dsp.Compressor, snapshot.Dsp.Compressor, transaction);
        state.Dsp.Polish.Thick.PrepareBaseline(snapshot.Dsp.Polish.Thick, transaction);
        state.Dsp.Polish.Air.PrepareBaseline(snapshot.Dsp.Polish.Air, transaction);
        state.Dsp.Polish.Bypass.PrepareBaseline(snapshot.Dsp.Polish.Bypass, transaction);
        state.Dsp.Polish.Solo.PrepareBaseline(snapshot.Dsp.Polish.Solo, transaction);
        state.Dsp.Equalizer.Bypass.PrepareBaseline(
            snapshot.Dsp.Equalizer.Bypass,
            transaction);
        state.Dsp.Equalizer.Solo.PrepareBaseline(
            snapshot.Dsp.Equalizer.Solo,
            transaction);
        for (var bankIndex = 0; bankIndex < DspConstants.BankCount; bankIndex++)
        {
            var source = snapshot.Dsp.Equalizer.Banks[bankIndex];
            var target = state.Dsp.EqualizerBanks[bankIndex];
            target.Bypass.PrepareBaseline(source.Bypass, transaction);
            target.Solo.PrepareBaseline(source.Solo, transaction);
            ApplyFilters(target.Filters, source.Filters, transaction);
        }
        ApplyOutput(state.Dsp.OutputGain, snapshot.Dsp.Output, transaction);
        transaction.Commit();
    }

    private static void ApplyInput(
        InputState target,
        PersistentInput source,
        StateHistoryTransaction transaction)
    {
        target.Level.PrepareBaseline(source.Level, transaction);
        target.Target.PrepareBaseline(source.Target, transaction);
        target.Width.PrepareBaseline(source.Width, transaction);
        target.Leveler.PrepareBaseline(source.Leveler, transaction);
        target.Bypass.PrepareBaseline(source.Bypass, transaction);
    }

    private static void ApplyOutput(
        OutputState target,
        PersistentOutput source,
        StateHistoryTransaction transaction)
    {
        target.Level.PrepareBaseline(source.Level, transaction);
        target.Target.PrepareBaseline(source.Target, transaction);
        target.Limiter.PrepareBaseline(source.Limiter, transaction);
        target.Bypass.PrepareBaseline(source.Bypass, transaction);
    }

    private static void ApplySaturator(
        SaturatorState target,
        PersistentSaturator source,
        StateHistoryTransaction transaction)
    {
        target.Drive.PrepareBaseline(source.Drive, transaction);
        target.Curve.PrepareBaseline(source.Curve, transaction);
        target.Split.PrepareBaseline(source.Split, transaction);
        target.OutputDb.PrepareBaseline(source.Output, transaction);
        target.Bypass.PrepareBaseline(source.Bypass, transaction);
        target.Solo.PrepareBaseline(source.Solo, transaction);
        ApplyDetector(target.Detector, source.Detector, transaction);
    }

    private static void ApplyCompressor(
        CompressorState target,
        PersistentCompressor source,
        StateHistoryTransaction transaction)
    {
        target.Attack.PrepareBaseline(source.Attack, transaction);
        target.Sustain.PrepareBaseline(source.Sustain, transaction);
        target.Compression.PrepareBaseline(source.Compression, transaction);
        target.Character.PrepareBaseline(source.Character, transaction);
        target.Parallel.PrepareBaseline(source.Parallel, transaction);
        target.OutputDb.PrepareBaseline(source.Output, transaction);
        target.Bypass.PrepareBaseline(source.Bypass, transaction);
        target.Solo.PrepareBaseline(source.Solo, transaction);
        ApplyDetector(target.Detector, source.Detector, transaction);
    }

    private static void ApplyDetector(
        DetectorState target,
        PersistentDetector source,
        StateHistoryTransaction transaction)
    {
        target.Listen.PrepareBaseline(source.Listen, transaction);
        ApplyFilters(target.Filters, source.Filters, transaction);
    }

    private static void ApplyFilters(
        IReadOnlyList<FilterState> target,
        IReadOnlyList<PersistentFilter> source,
        StateHistoryTransaction transaction)
    {
        for (var index = 0; index < target.Count; index++)
        {
            if (target[index].FrequencyHz is { } frequency)
            {
                frequency.PrepareBaseline(source[index].FrequencyHz
                    ?? throw new InvalidDataException("Filter frequency is missing."),
                    transaction);
            }
            else if (source[index].FrequencyHz is not null)
            {
                throw new InvalidDataException("Filter frequency is not supported.");
            }
            if (target[index].Q is { } q)
            {
                q.PrepareBaseline(source[index].Q
                    ?? throw new InvalidDataException("Filter Q is missing."),
                    transaction);
            }
            else if (source[index].Q is not null)
            {
                throw new InvalidDataException("Filter Q is not supported.");
            }
            target[index].GainDb.PrepareBaseline(
                source[index].GainDb,
                transaction);
            target[index].Bypass.PrepareBaseline(
                source[index].Bypass,
                transaction);
            target[index].Solo.PrepareBaseline(
                source[index].Solo,
                transaction);
        }
    }

    private static void Validate(PersistentStateV2 snapshot)
    {
        if (snapshot.Schema != CurrentSchema || snapshot.Instance is null ||
            snapshot.Banks is null ||
            snapshot.Banks.Length != DspConstants.BankCount ||
            snapshot.Dsp is null || snapshot.Dsp.Input is null ||
            snapshot.Dsp.Saturator is null || snapshot.Dsp.Saturator.Detector is null ||
            snapshot.Dsp.Compressor is null || snapshot.Dsp.Compressor.Detector is null ||
            snapshot.Dsp.Equalizer is null || snapshot.Dsp.Polish is null || snapshot.Dsp.Output is null ||
            snapshot.Dsp.Equalizer.Banks is null ||
            snapshot.Dsp.Equalizer.Banks.Length != DspConstants.BankCount)
        {
            throw new InvalidDataException("Unsupported persistence payload.");
        }

        if (snapshot.Banks.Any(bank => bank is null) ||
            snapshot.Dsp!.Equalizer!.Banks.Any(bank => bank is null))
        {
            throw new InvalidDataException("Invalid persistence object.");
        }

        var groups = snapshot.Banks.Select(bank => bank.Group).ToArray();
        if (groups[6] != 0 ||
            groups.Take(6).Any(group => group == 0) ||
            groups.Where(group => group is not null)
                .GroupBy(group => group)
                .Any(group => group.Count() > 1))
        {
            throw new InvalidDataException("Invalid bank topology.");
        }

        var dsp = snapshot.Dsp!;
        var equalizer = dsp.Equalizer!;
        var saturatorDetector = dsp.Saturator!.Detector!;
        var compressorDetector = dsp.Compressor!.Detector!;
        ValidateRange(dsp.Input!.Level, DspParameterRanges.GainDb);
        ValidateRange(dsp.Input.Target, DspParameterRanges.TargetDb);
        ValidateRange(dsp.Input.Width, DspParameterRanges.Width);
        ValidateRange(dsp.Output!.Level, DspParameterRanges.GainDb);
        ValidateRange(dsp.Output.Target, DspParameterRanges.TargetDb);
        ValidateRange(dsp.Saturator.Drive, DspParameterRanges.Drive);
        ValidateRange(dsp.Saturator.Curve, DspParameterRanges.Curve);
        ValidateRange(dsp.Saturator.Output, DspParameterRanges.OutputDb);
        ValidateRange(dsp.Compressor.Attack, DspParameterRanges.Macro);
        ValidateRange(dsp.Compressor.Sustain, DspParameterRanges.Macro);
        ValidateRange(dsp.Compressor.Compression, DspParameterRanges.Macro);
        ValidateRange(dsp.Compressor.Output, DspParameterRanges.OutputDb);
        if (dsp.Compressor.Character is < 0 or > 2)
        {
            throw new InvalidDataException("Invalid compressor character.");
        }
        ValidateRange(dsp.Polish.Thick, DspParameterRanges.Macro);
        ValidateRange(dsp.Polish.Air, DspParameterRanges.Macro);
        if (equalizer.Banks.Any(bank => bank.Filters is null) ||
            saturatorDetector.Filters is null ||
            compressorDetector.Filters is null)
        {
            throw new InvalidDataException("Invalid filter array.");
        }

        ValidateFilters(equalizer.Banks.SelectMany(bank => bank.Filters!));
        ValidateFilters(saturatorDetector.Filters!);
        ValidateFilters(compressorDetector.Filters!);
        if (equalizer.Banks.Any(bank =>
                bank.Filters!.Length != DspConstants.EqualizerFilterCount) ||
            saturatorDetector.Filters!.Length !=
                DspConstants.DetectorFilterCount ||
            compressorDetector.Filters!.Length !=
                DspConstants.DetectorFilterCount)
        {
            throw new InvalidDataException("Invalid persistence array length.");
        }
    }

    private static void ValidateFilters(IEnumerable<PersistentFilter> filters)
    {
        foreach (var filter in filters)
        {
            if (filter is null)
            {
                throw new InvalidDataException("Invalid filter object.");
            }
            ValidateRange(filter.GainDb, DspParameterRanges.FilterGainDb);
            if (filter.FrequencyHz is { } frequency)
            {
                ValidateRange(frequency, DspParameterRanges.FrequencyHz);
            }
            if (filter.Q is { } q)
            {
                ValidateRange(q, DspParameterRanges.Q);
            }
        }
    }

    private static void ValidateRange(float value, FloatRange range)
    {
        if (float.IsNaN(value) || float.IsInfinity(value) || !range.Contains(value))
        {
            throw new InvalidDataException("Persistence value is outside its range.");
        }
    }
}

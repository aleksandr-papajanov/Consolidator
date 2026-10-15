using System.Text.Json;

using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Services.Instances;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.State.History;

namespace Consolidator.Managed.Core.Services.Persistence;

internal sealed class InstancePersistenceService
{
    private const int CurrentSchema = 9;
    private readonly InstanceRegistry _instances;
    private readonly IOperationGate _operationGate;
    private readonly IPersistenceChangeSink _persistenceChanges;
    private readonly DspStateChangeTracker _dspChanges;

    internal InstancePersistenceService(
        InstanceRegistry instances,
        IOperationGate operationGate,
        IPersistenceChangeSink persistenceChanges,
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
                PersistenceJsonContext.Default.PersistentState);
        }
    }

    public void Restore(InstanceId instanceId, ReadOnlySpan<byte> utf8Json)
    {
        var snapshot = JsonSerializer.Deserialize(
            utf8Json,
            PersistenceJsonContext.Default.PersistentState)
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

    private static PersistentState CreateSnapshot(ManagedState state) => new(
        CurrentSchema,
        new PersistentInstance(
            state.Instance.Mute.Value,
            state.Instance.Solo.Value,
            state.Instance.Bypass.Value),
        state.Instance.Banks
            .Select(bank => new PersistentBank(bank.Group.Value?.Value))
            .ToArray(),
        new PersistentDsp(
            CreateGainStaging(state.Dsp.GainStaging),
            CreateSaturator(state.Dsp.Saturator),
            CreateShaper(state.Dsp.Shaper),
            CreateCompression(state.Dsp.Compression),
            CreateEqualizer(state.Dsp.Equalizer, state.Instance.Banks),
            CreatePolish(state.Dsp.Polish)));

    private static PersistentGainStaging CreateGainStaging(GainStagingState value) => new(
        value.InputLevel.Value, value.InputTarget.Value, value.InputLeveler.Value,
        value.OutputLevel.Value, value.OutputTarget.Value, value.OutputLeveler.Value,
        value.Limit.Value, value.Width.Value, value.Bypass.Value,
        CreateDetector(value.InputDetector), CreateDetector(value.OutputDetector));

    private static PersistentSaturator CreateSaturator(SaturatorState value) => new(
        value.Drive.Value, value.Curve.Value, value.Split.Value,
        value.OutputDb.Value, value.Bypass.Value,
        CreateDetector(value.Detector));

    private static PersistentShaper CreateShaper(ShaperState value) =>
        new(
            value.Attack.Value,
            value.Sustain.Value,
            value.OutputDb.Value,
            value.Bypass.Value);

    private static PersistentCompression CreateCompression(CompressionState value) =>
        new(
            value.Type.Value,
            value.Reduction.Value,
            value.OutputDb.Value,
            value.Attack.Value,
            value.Release.Value,
            value.Ratio.Value,
            value.Bypass.Value,
            value.Parallel.Value,
            value.Profiles.Select(profile => new PersistentCompressionProfile(
                profile.Reduction.Value,
                profile.OutputDb.Value,
                profile.Attack.Value,
                profile.Release.Value,
                profile.Ratio.Value)).ToArray(),
            CreateDetector(value.Detector));

    private static PersistentPolish CreatePolish(PolishState value) =>
        new(value.Thick.Value, value.Air.Value, value.Bypass.Value);

    private static PersistentDetector CreateDetector(DetectorState value) => new(
        value.Filters.Select(CreateFilter).ToArray());

    private static PersistentEqualizer CreateEqualizer(
        EqualizerState equalizer,
        IReadOnlyList<BankState> banks) => new(
        equalizer.Bypass.Value,
        banks.Select(bank => new PersistentEqualizerBank(
            bank.Bypass.Value,
            bank.Equalizer.Filters.Select(CreateFilter).ToArray())).ToArray());

    private static PersistentFilter CreateFilter(FilterState value) => value switch
    {
        GainFilterState gain => new(
            null,
            null,
            gain.GainDb.Value,
            gain.Bypass.Value),
        FixedQFilterState fixedQ => new(
            fixedQ.FrequencyHz.Value,
            null,
            fixedQ.GainDb.Value,
            fixedQ.Bypass.Value),
        BellFilterState bell => new(
            bell.FrequencyHz.Value,
            bell.Q.Value,
            bell.GainDb.Value,
            bell.Bypass.Value),
        _ => throw new InvalidOperationException("Unknown filter state.")
    };

    private static void Apply(ManagedState state, PersistentState snapshot)
    {
        using var transaction = new StateHistoryTransaction();
        state.Instance.Mute.PrepareBaseline(snapshot.Instance.Mute, transaction);
        state.Instance.Solo.PrepareBaseline(snapshot.Instance.Solo, transaction);
        state.Instance.Bypass.PrepareBaseline(snapshot.Instance.Bypass, transaction);
        for (var index = 0; index < DspConstants.BankCount; index++)
        {
            state.Instance.Banks[index].Group.PrepareBaseline(
                snapshot.Banks[index].Group is { } group ? new GroupId(group) : null,
                transaction);
        }

        ApplyGainStaging(state.Dsp.GainStaging, snapshot.Dsp.GainStaging, transaction);
        ApplySaturator(state.Dsp.Saturator, snapshot.Dsp.Saturator, transaction);
        ApplyShaper(state.Dsp.Shaper, snapshot.Dsp.Shaper, transaction);
        ApplyCompression(state.Dsp.Compression, snapshot.Dsp.Compression, transaction);
        state.Dsp.Polish.Thick.PrepareBaseline(snapshot.Dsp.Polish.Thick, transaction);
        state.Dsp.Polish.Air.PrepareBaseline(snapshot.Dsp.Polish.Air, transaction);
        state.Dsp.Polish.Bypass.PrepareBaseline(snapshot.Dsp.Polish.Bypass, transaction);
        state.Dsp.Equalizer.Bypass.PrepareBaseline(
            snapshot.Dsp.Equalizer.Bypass,
            transaction);
        for (var bankIndex = 0; bankIndex < DspConstants.BankCount; bankIndex++)
        {
            var source = snapshot.Dsp.Equalizer.Banks[bankIndex];
            var target = state.Instance.Banks[bankIndex];
            target.Bypass.PrepareBaseline(source.Bypass, transaction);
            ApplyFilters(target.Equalizer.Filters, source.Filters, transaction);
        }
        transaction.Commit();
    }

    private static void ApplyGainStaging(
        GainStagingState target,
        PersistentGainStaging source,
        StateHistoryTransaction transaction)
    {
        target.InputLevel.PrepareBaseline(source.InputLevel, transaction);
        target.InputTarget.PrepareBaseline(source.InputTarget, transaction);
        target.InputLeveler.PrepareBaseline(source.InputLeveler, transaction);
        target.OutputLevel.PrepareBaseline(source.OutputLevel, transaction);
        target.OutputTarget.PrepareBaseline(source.OutputTarget, transaction);
        target.OutputLeveler.PrepareBaseline(source.OutputLeveler, transaction);
        target.Limit.PrepareBaseline(source.Limit, transaction);
        target.Width.PrepareBaseline(source.Width, transaction);
        target.Bypass.PrepareBaseline(source.Bypass, transaction);
        ApplyDetector(target.InputDetector, source.InputDetector, transaction);
        ApplyDetector(target.OutputDetector, source.OutputDetector, transaction);
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
        ApplyDetector(target.Detector, source.Detector, transaction);
    }

    private static void ApplyShaper(
        ShaperState target,
        PersistentShaper source,
        StateHistoryTransaction transaction)
    {
        target.Attack.PrepareBaseline(source.Attack, transaction);
        target.Sustain.PrepareBaseline(source.Sustain, transaction);
        target.OutputDb.PrepareBaseline(source.Output, transaction);
        target.Bypass.PrepareBaseline(source.Bypass, transaction);
    }

    private static void ApplyCompression(
        CompressionState target,
        PersistentCompression source,
        StateHistoryTransaction transaction)
    {
        target.Type.PrepareBaseline(source.Type, transaction);
        target.Reduction.PrepareBaseline(source.Reduction, transaction);
        target.OutputDb.PrepareBaseline(source.Output, transaction);
        target.Attack.PrepareBaseline(source.Attack, transaction);
        target.Release.PrepareBaseline(source.Release, transaction);
        target.Ratio.PrepareBaseline(source.Ratio, transaction);
        target.Bypass.PrepareBaseline(source.Bypass, transaction);
        for (var index = 0; index < target.Profiles.Length; index++)
        {
            var profile = target.Profiles[index];
            var sourceProfile = source.Profiles[index];
            profile.Reduction.PrepareBaseline(sourceProfile.Reduction, transaction);
            profile.OutputDb.PrepareBaseline(sourceProfile.Output, transaction);
            profile.Attack.PrepareBaseline(sourceProfile.Attack, transaction);
            profile.Release.PrepareBaseline(sourceProfile.Release, transaction);
            profile.Ratio.PrepareBaseline(sourceProfile.Ratio, transaction);
        }
        target.Parallel.PrepareBaseline(source.Parallel, transaction);
        ApplyDetector(target.Detector, source.Detector, transaction);
    }

    private static void ApplyDetector(
        DetectorState target,
        PersistentDetector source,
        StateHistoryTransaction transaction)
    {
        ApplyFilters(target.Filters, source.Filters, transaction);
    }

    private static void ApplyFilters(
        IReadOnlyList<FilterState> target,
        IReadOnlyList<PersistentFilter> source,
        StateHistoryTransaction transaction)
    {
        for (var index = 0; index < target.Count; index++)
        {
            switch (target[index])
            {
                case GainFilterState:
                    if (source[index].FrequencyHz is not null || source[index].Q is not null)
                    {
                        throw new InvalidDataException("Filter parameters are not supported.");
                    }
                    break;
                case FixedQFilterState fixedQ:
                    fixedQ.FrequencyHz.PrepareBaseline(
                        source[index].FrequencyHz
                            ?? throw new InvalidDataException("Filter frequency is missing."),
                        transaction);
                    if (source[index].Q is not null)
                    {
                        throw new InvalidDataException("Filter Q is not supported.");
                    }
                    break;
                case BellFilterState bell:
                    bell.FrequencyHz.PrepareBaseline(
                        source[index].FrequencyHz
                            ?? throw new InvalidDataException("Filter frequency is missing."),
                        transaction);
                    bell.Q.PrepareBaseline(
                        source[index].Q
                            ?? throw new InvalidDataException("Filter Q is missing."),
                        transaction);
                    break;
                default:
                    throw new InvalidDataException("Unknown filter state.");
            }
            target[index].GainDb.PrepareBaseline(
                source[index].GainDb,
                transaction);
            target[index].Bypass.PrepareBaseline(
                source[index].Bypass,
                transaction);
        }
    }

    private static void Validate(PersistentState snapshot)
    {
        if (snapshot.Schema != CurrentSchema || snapshot.Instance is null ||
            snapshot.Banks is null ||
            snapshot.Banks.Length != DspConstants.BankCount ||
            snapshot.Dsp is null || snapshot.Dsp.GainStaging is null || snapshot.Dsp.GainStaging.InputDetector is null ||
            snapshot.Dsp.GainStaging.OutputDetector is null ||
            snapshot.Dsp.Saturator is null || snapshot.Dsp.Saturator.Detector is null ||
            snapshot.Dsp.Shaper is null ||
            snapshot.Dsp.Compression is null || snapshot.Dsp.Compression.Profiles is null ||
            snapshot.Dsp.Compression.Detector is null ||
            snapshot.Dsp.Equalizer is null || snapshot.Dsp.Polish is null ||
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
        var compressionDetector = dsp.Compression!.Detector!;
        var gainStaging = dsp.GainStaging!;
        var definitions = StateValueDefinitions.GainStaging;
        ValidateRange(gainStaging.InputLevel, definitions.InputLevel);
        ValidateRange(gainStaging.InputTarget, definitions.InputTarget);
        ValidateRange(gainStaging.OutputLevel, definitions.OutputLevel);
        ValidateRange(gainStaging.OutputTarget, definitions.OutputTarget);
        ValidateRange(gainStaging.Limit, definitions.Limit);
        ValidateRange(gainStaging.Width, definitions.Width);
        ValidateRange(dsp.Saturator.Drive, StateValueDefinitions.Saturator.Drive);
        ValidateRange(dsp.Saturator.Curve, StateValueDefinitions.Saturator.Curve);
        ValidateRange(dsp.Saturator.Output, StateValueDefinitions.Saturator.OutputDb);
        ValidateRange(dsp.Shaper.Attack, StateValueDefinitions.Shaper.Attack);
        ValidateRange(dsp.Shaper.Sustain, StateValueDefinitions.Shaper.Sustain);
        ValidateRange(dsp.Shaper.Output, StateValueDefinitions.Shaper.OutputDb);
        ValidateRange(dsp.Compression.Reduction, StateValueDefinitions.Compression.Reduction);
        ValidateRange(dsp.Compression.Output, StateValueDefinitions.Compression.OutputDb);
        ValidateRange(dsp.Compression.Attack, StateValueDefinitions.Compression.Attack);
        ValidateRange(dsp.Compression.Release, StateValueDefinitions.Compression.Release);
        ValidateRange(dsp.Compression.Ratio, StateValueDefinitions.Compression.Ratio);
        if (dsp.Compression.Type is < 0 or > 3)
        {
            throw new InvalidDataException("Invalid compression type.");
        }
        if (dsp.Compression.Profiles.Length != 4 ||
            dsp.Compression.Profiles.Any(profile => profile is null))
        {
            throw new InvalidDataException("Invalid compression profile array.");
        }
        foreach (var profile in dsp.Compression.Profiles)
        {
            ValidateRange(profile.Reduction, StateValueDefinitions.Compression.Reduction);
            ValidateRange(profile.Output, StateValueDefinitions.Compression.OutputDb);
            ValidateRange(profile.Attack, StateValueDefinitions.Compression.Attack);
            ValidateRange(profile.Release, StateValueDefinitions.Compression.Release);
            ValidateRange(profile.Ratio, StateValueDefinitions.Compression.Ratio);
        }
        var activeProfile = dsp.Compression.Profiles[dsp.Compression.Type];
        if (activeProfile.Reduction != dsp.Compression.Reduction ||
            activeProfile.Output != dsp.Compression.Output ||
            activeProfile.Attack != dsp.Compression.Attack ||
            activeProfile.Release != dsp.Compression.Release ||
            activeProfile.Ratio != dsp.Compression.Ratio)
        {
            throw new InvalidDataException("Active compression values do not match the selected profile.");
        }
        ValidateRange(dsp.Polish.Thick, StateValueDefinitions.Polish.Thick);
        ValidateRange(dsp.Polish.Air, StateValueDefinitions.Polish.Air);
        if (equalizer.Banks.Any(bank => bank.Filters is null) ||
            gainStaging.InputDetector.Filters is null ||
            gainStaging.OutputDetector.Filters is null ||
            saturatorDetector.Filters is null ||
            compressionDetector.Filters is null)
        {
            throw new InvalidDataException("Invalid filter array.");
        }

        ValidateFilters(equalizer.Banks.SelectMany(bank => bank.Filters!));
        ValidateFilters(gainStaging.InputDetector.Filters!);
        ValidateFilters(gainStaging.OutputDetector.Filters!);
        ValidateFilters(saturatorDetector.Filters!);
        ValidateFilters(compressionDetector.Filters!);
        if (equalizer.Banks.Any(bank =>
                bank.Filters!.Length != DspConstants.EqualizerFilterCount) ||
            gainStaging.InputDetector.Filters!.Length != DspConstants.DetectorFilterCount ||
            gainStaging.OutputDetector.Filters!.Length != DspConstants.DetectorFilterCount ||
            saturatorDetector.Filters!.Length !=
                DspConstants.DetectorFilterCount ||
            compressionDetector.Filters!.Length !=
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
            ValidateRange(
                filter.GainDb,
                StateValueDefinitions.EqualizerDefinitions[0].Gain.Definition);
            if (filter.FrequencyHz is { } frequency)
            {
                ValidateRange(
                    frequency,
                    ((FrequencyFilterDefinition)StateValueDefinitions.EqualizerDefinitions[1]).Frequency.Definition);
            }
            if (filter.Q is { } q)
            {
                ValidateRange(
                    q,
                    ((BellFilterDefinition)StateValueDefinitions.EqualizerDefinitions[4]).Q.Definition);
            }
        }
    }

    private static void ValidateRange(
        float value,
        StateValueDefinition<float> definition)
    {
        if (float.IsNaN(value) ||
            float.IsInfinity(value) ||
            definition.PhysicalRange is not { } range ||
            !range.Contains(value))
        {
            throw new InvalidDataException("Persistence value is outside its range.");
        }
    }
}

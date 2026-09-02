using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.Services.Abstractions;
using Consolidator.Managed.Core.Settings;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Observers;

namespace Consolidator.Managed.Core.State.Composition;

internal sealed class ManagedStateBuilder
{
    private readonly StateRegistry<InstanceId> _stateRegistry;
    private readonly StateValueFactory _stateValues;
    private readonly AudibilityObserver _audibilityObserver;
    private readonly StateTopologyObserver _topologyObserver;
    private readonly IActivityStatusSink _activitySink;

    public ManagedStateBuilder(
        StateRegistry<InstanceId> stateRegistry,
        StateValueFactory stateValues,
        AudibilityObserver audibilityObserver,
        StateTopologyObserver topologyObserver,
        IActivityStatusSink activitySink)
    {
        ArgumentNullException.ThrowIfNull(stateRegistry);
        ArgumentNullException.ThrowIfNull(stateValues);
        ArgumentNullException.ThrowIfNull(audibilityObserver);
        ArgumentNullException.ThrowIfNull(topologyObserver);
        ArgumentNullException.ThrowIfNull(activitySink);

        _stateRegistry = stateRegistry;
        _stateValues = stateValues;
        _audibilityObserver = audibilityObserver;
        _topologyObserver = topologyObserver;
        _activitySink = activitySink;
    }

    public ManagedState Build(InstanceId instanceId)
    {
        _stateRegistry.CreateRoot(instanceId);
        try
        {
            var runtime = new DspRuntimeState();
            var context = new StateModelContext(instanceId, _stateValues, runtime);
            var transient = new InstanceTransientState(instanceId, _topologyObserver);
            var instance = CreateInstance(context);
            var dsp = CreateDsp(context);
            var root = _stateRegistry.GetRoot(instanceId);
            return new ManagedState(instance, transient, dsp, dsp.Activity, runtime, root);
        }
        catch
        {
            _stateRegistry.RemoveRoot(instanceId);
            throw;
        }
    }

    private InstanceState CreateInstance(StateModelContext context)
    {
        var instancePath = new StatePath([StateNodeIds.Instance]);
        var label = CreateInstanceValue(
            context,
            instancePath.Append(StateNodeIds.Label),
            StateValueDefinitions.Common.Label);
        var mute = CreateInstanceValue(
            context,
            instancePath.Append(StateNodeIds.Mute),
            StateValueDefinitions.Common.CopyValueWithoutHistory,
            _audibilityObserver.ObserveMute(context.InstanceId, context.Runtime));
        var solo = CreateInstanceValue(
            context,
            instancePath.Append(StateNodeIds.Solo),
            StateValueDefinitions.Common.CopyValueWithoutHistory,
            _audibilityObserver.ObserveSolo(context.InstanceId, context.Runtime));
        var bypass = CreateInstanceValue(
            context,
            instancePath.Append(StateNodeIds.Bypass),
            StateValueDefinitions.Common.CopyValueWithoutHistory,
            new StateProjectionObserver<bool>(value => context.Runtime.InstanceBypass = value));
        var banks = Enumerable.Range(0, DspConstants.BankCount)
            .Select(index => CreateBank(
                context,
                instancePath.Append(StateNodeIds.Bank).Append(StateNodeIds.BankAt(index)),
                (BankId)index))
            .ToArray();
        return new InstanceState(context.InstanceId, label, mute, solo, bypass, banks);
    }

    private BankState CreateBank(StateModelContext context, StatePath path, BankId id)
    {
        var initialGroup = id == BankId.Bank6 ? new GroupId(0) : (GroupId?)null;
        var group = context.Values.Create(
            StateValueCreationContext.Bank(
                context.InstanceId,
                path.Append(StateNodeIds.Group),
                StateValueEditScope.Local),
            new StateValueDefinition<GroupId?>(initialGroup),
            observers: [_topologyObserver.ObserveBankGroup(new BankAddress(context.InstanceId, (int)id))]);
        return new BankState(id, group);
    }

    private DspState CreateDsp(StateModelContext context)
    {
        var dspPath = new StatePath([StateNodeIds.Dsp]);
        var activity = new ActivityObserver(context.InstanceId, _activitySink);
        var inputGain = CreateInput(
            context,
            dspPath.Append(StateNodeIds.InputGain),
            activity);
        var saturator = CreateSaturator(
            context,
            dspPath.Append(StateNodeIds.Saturator),
            activity);
        var compressor = CreateCompressor(
            context,
            dspPath.Append(StateNodeIds.Compressor),
            activity);
        var polish = CreatePolish(context, dspPath.Append(StateNodeIds.Polish));
        var equalizer = CreateEqualizer(context, dspPath.Append(StateNodeIds.Equalizer));
        var equalizerBanks = Enumerable.Range(0, DspConstants.BankCount)
            .Select(index => CreateEqualizerBank(
                context,
                dspPath
                    .Append(StateNodeIds.Equalizer)
                    .Append(StateNodeIds.EqualizerBank)
                    .Append(StateNodeIds.BankAt(index)),
                index,
                activity))
            .ToArray();
        var outputGain = CreateOutput(context, dspPath.Append(StateNodeIds.OutputGain));
        var dsp = new DspState(
            inputGain,
            saturator,
            compressor,
            polish,
            equalizer,
            equalizerBanks,
            activity,
            outputGain);
        activity.Initialize(dsp);
        return dsp;
    }

    private InputState CreateInput(
        StateModelContext context,
        StatePath path,
        ActivityObserver activity)
    {
        var runtime = context.Runtime;
        return new InputState(
            CreateInstanceValue(context, path.Append(StateNodeIds.Level), StateValueDefinitions.Input.Level,
                new StateProjectionObserver<float>(value => runtime.InputLevel = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Target), StateValueDefinitions.Input.Target,
                new StateProjectionObserver<float>(value => runtime.InputTarget = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Width), StateValueDefinitions.Input.Width,
                new StateProjectionObserver<float>(value => runtime.InputWidth = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Leveler), StateValueDefinitions.Common.CopyValue,
                new StateProjectionObserver<bool>(value => runtime.InputLeveler = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Bypass), StateValueDefinitions.Common.CopyValueWithoutHistory,
                new StateProjectionObserver<bool>(value => runtime.InputGainBypass = value)),
            CreateDetector(
                context,
                path.Append(StateNodeIds.Detector),
                runtime,
                0,
                index => activity.ObserveActive(active => runtime[index].Active = active ? 1U : 0U)));
    }

    private FilterState CreateFilter(
        StateModelContext context,
        StateValueCreationContext valueContext,
        FilterDefinition definition,
        IReadOnlyList<IStateValueObserver<bool>> bypassObservers,
        IReadOnlyList<IStateValueObserver<float>> gainObservers,
        Action<FilterDefinition> initialize,
        Action<float> setFrequency,
        Action<float> setGain,
        Action<float> setQ)
    {
        initialize(definition);
        var gain = CreateParameter(
            context,
            valueContext with { Path = valueContext.Path.Append(definition.Gain.Node) },
            definition.Gain.Definition,
            gainObservers
                .Append(new StateProjectionObserver<float>(setGain))
                .ToArray());
        var bypass = context.Values.Create(
            valueContext with { Path = valueContext.Path.Append(StateNodeIds.Bypass) },
            StateValueDefinitions.Common.CopyValueWithoutHistory,
            bypassObservers.ToArray());
        var solo = context.Values.Create(
            valueContext with { Path = valueContext.Path.Append(StateNodeIds.Solo) },
            StateValueDefinitions.Common.CopyValueWithoutHistory);

        return definition switch
        {
            GainFilterDefinition => new GainFilterState(definition, gain, bypass, solo),
            FixedQFilterDefinition fixedQ => new FixedQFilterState(
                definition, gain, bypass, solo,
                CreateParameter(
                    context,
                    valueContext with { Path = valueContext.Path.Append(fixedQ.Frequency.Node) },
                    fixedQ.Frequency.Definition,
                    new StateProjectionObserver<float>(setFrequency))),
            BellFilterDefinition bell => new BellFilterState(
                definition, gain, bypass, solo,
                CreateParameter(
                    context,
                    valueContext with { Path = valueContext.Path.Append(bell.Frequency.Node) },
                    bell.Frequency.Definition,
                    new StateProjectionObserver<float>(setFrequency)),
                CreateParameter(
                    context,
                    valueContext with { Path = valueContext.Path.Append(bell.Q.Node) },
                    bell.Q.Definition,
                    new StateProjectionObserver<float>(setQ))),
            _ => throw new InvalidOperationException("Unknown filter definition.")
        };
    }

    private static DspFilterType GetDspFilterType(FilterDefinition definition) => definition switch
    {
        GainFilterDefinition => DspFilterType.Gain,
        TiltFilterDefinition => DspFilterType.Tilt,
        LowShelfFilterDefinition => DspFilterType.LowShelf,
        HighShelfFilterDefinition => DspFilterType.HighShelf,
        BellFilterDefinition => DspFilterType.Bell,
        _ => throw new InvalidOperationException("Unknown filter definition.")
    };

    private static float GetFixedQ(FilterDefinition definition) =>
        definition is FixedQFilterDefinition fixedQ ? fixedQ.FixedQ : 0.0F;

    private static StateValue<TValue> CreateParameter<TValue>(
        StateModelContext context,
        StateValueCreationContext valueContext,
        StateValueDefinition<TValue> definition,
        params IStateValueObserver<TValue>[] observers)
    {
        return context.Values.Create(valueContext, definition, observers);
    }

    private DetectorState CreateDetector(
        StateModelContext context,
        StatePath path,
        DspRuntimeState runtime,
        int runtimeOffset,
        Func<int, IStateValueObserver<bool>> observerFactory)
    {
        return new DetectorState(
            StateValueDefinitions.DetectorDefinitions
                .Select((definition, index) => CreateFilter(
                    context,
                    StateValueCreationContext.Instance(
                        context.InstanceId,
                        path.Append(StateNodeIds.Filter).Append(StateNodeIds.FilterAt(index))),
                    definition,
                    [observerFactory(index)],
                    [],
                    filterDefinition =>
                    {
                        ref var filter = ref runtime[runtimeOffset + index];
                        filter.Type = (uint)GetDspFilterType(filterDefinition);
                        filter.FixedQ = GetFixedQ(filterDefinition);
                    },
                    frequency => runtime[runtimeOffset + index].FrequencyHz = frequency,
                    gain => runtime[runtimeOffset + index].GainDb = gain,
                    q => runtime[runtimeOffset + index].Q = q))
                .ToArray());
    }

    private EqualizerBankState CreateEqualizerBank(
            StateModelContext context,
            StatePath path,
            int bankIndex,
            ActivityObserver activity)
        {
            var runtime = context.Runtime;
            var bank = runtime.EqualizerBanks[bankIndex];
            var bypass = CreateBankValue(
                context,
                path.Append(StateNodeIds.Bypass),
                StateValueDefinitions.Common.CopyValueWithoutHistory,
                new StateProjectionObserver<bool>(value =>
                    bank.Active = !value),
                activity.ObserveBankBypass(bankIndex));
            var solo = CreateBankValue(
                context,
                path.Append(StateNodeIds.Solo),
                StateValueDefinitions.Common.CopyValueWithoutHistory);
            var filters = StateValueDefinitions.EqualizerDefinitions
                .Select((definition, index) => CreateFilter(
                    context,
                    StateValueCreationContext.Bank(
                        context.InstanceId,
                        path.Append(StateNodeIds.Filter).Append(StateNodeIds.FilterAt(index))),
                        definition,
                        [
                            activity.ObserveActive(activeValue => bank[index].Active = activeValue ? 1U : 0U),
                            activity.ObserveFilterBypass(bankIndex, index)
                        ],
                        [activity.ObserveFilterGain(bankIndex, index)],
                        filterDefinition =>
                        {
                            ref var filter = ref bank[index];
                            filter.Type = (uint)GetDspFilterType(filterDefinition);
                            filter.FixedQ = GetFixedQ(filterDefinition);
                        },
                        frequency => bank[index].FrequencyHz = frequency,
                        gain => bank[index].GainDb = gain,
                        q =>
                        {
                            bank[index].Q = q;
                        }))
                .ToArray();
            return new EqualizerBankState(
                bypass,
                solo,
                filters);
        }

    private PolishState CreatePolish(StateModelContext context, StatePath path)
    {
        var runtime = context.Runtime;
        return new PolishState(
            CreateInstanceValue(context, path.Append(StateNodeIds.Thick), StateValueDefinitions.Polish.Thick,
                new StateProjectionObserver<float>(value => runtime.PolishThick = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Air), StateValueDefinitions.Polish.Air,
                new StateProjectionObserver<float>(value => runtime.PolishAir = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Bypass), StateValueDefinitions.Common.CopyValueWithoutHistory,
                new StateProjectionObserver<bool>(value => runtime.PolishBypass = value)));
    }

    private SaturatorState CreateSaturator(
        StateModelContext context,
        StatePath path,
        ActivityObserver activity)
    {
        var runtime = context.Runtime;
        return new SaturatorState(
            CreateInstanceValue(context, path.Append(StateNodeIds.Drive), StateValueDefinitions.Saturator.Drive,
                new StateProjectionObserver<float>(value => runtime.SaturatorDrive = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Curve), StateValueDefinitions.Saturator.Curve,
                new StateProjectionObserver<float>(value => runtime.SaturatorCurve = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Split), StateValueDefinitions.Saturator.Split,
                new StateProjectionObserver<bool>(value => runtime.SaturatorSplit = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Output), StateValueDefinitions.Saturator.OutputDb,
                new StateProjectionObserver<float>(value => runtime.SaturatorOutputDb = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Bypass), StateValueDefinitions.Common.CopyValueWithoutHistory,
                new StateProjectionObserver<bool>(value =>
                {
                    runtime.SaturatorBypass = value;
                    runtime.SaturatorActive = !value;
                })),
            CreateDetector(
                context,
                path.Append(StateNodeIds.Detector),
                runtime,
                DspConstants.DetectorFilterCount,
                index => activity.ObserveActive(active =>
                {
                    runtime[DspConstants.DetectorFilterCount + index].Active = active ? 1U : 0U;
                })));
    }

    private CompressorState CreateCompressor(
        StateModelContext context,
        StatePath path,
        ActivityObserver activity)
    {
        var runtime = context.Runtime;
        return new CompressorState(
            CreateInstanceValue(context, path.Append(StateNodeIds.Attack), StateValueDefinitions.Compressor.Attack,
                new StateProjectionObserver<float>(value => runtime.CompressorAttack = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Sustain), StateValueDefinitions.Compressor.Sustain,
                new StateProjectionObserver<float>(value => runtime.CompressorSustain = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Compression), StateValueDefinitions.Compressor.Compression,
                new StateProjectionObserver<float>(value => runtime.CompressorCompression = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Character), StateValueDefinitions.Compressor.Character,
                new StateProjectionObserver<int>(value => runtime.CompressorCharacter = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Parallel), StateValueDefinitions.Compressor.Parallel,
                new StateProjectionObserver<bool>(value => runtime.CompressorParallel = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Output), StateValueDefinitions.Compressor.OutputDb,
                new StateProjectionObserver<float>(value => runtime.CompressorOutputDb = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Bypass), StateValueDefinitions.Common.CopyValueWithoutHistory,
                new StateProjectionObserver<bool>(value =>
                {
                    runtime.CompressorBypass = value;
                    runtime.CompressorActive = !value;
                })),
            CreateDetector(
                context,
                path.Append(StateNodeIds.Detector),
                runtime,
                DspConstants.DetectorFilterCount * 2,
                index => activity.ObserveActive(active =>
                {
                    runtime[DspConstants.DetectorFilterCount * 2 + index].Active = active ? 1U : 0U;
                })));
    }

    private EqualizerState CreateEqualizer(StateModelContext context, StatePath path)
    {
        var runtime = context.Runtime;
        return new EqualizerState(
            CreateInstanceValue(context, path.Append(StateNodeIds.Bypass), StateValueDefinitions.Common.CopyValueWithoutHistory,
                new StateProjectionObserver<bool>(value =>
                {
                    runtime.EqualizerBypass = value;
                    runtime.EqualizerActive = !value;
                })));
    }

    private OutputState CreateOutput(StateModelContext context, StatePath path)
    {
        var runtime = context.Runtime;
        return new OutputState(
            CreateInstanceValue(context, path.Append(StateNodeIds.Level), StateValueDefinitions.Output.Level,
                new StateProjectionObserver<float>(value => runtime.OutputLevel = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Target), StateValueDefinitions.Output.Target,
                new StateProjectionObserver<float>(value => runtime.OutputTarget = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Limiter), StateValueDefinitions.Common.CopyValue,
                new StateProjectionObserver<bool>(value => runtime.OutputLimiter = value)),
            CreateInstanceValue(context, path.Append(StateNodeIds.Bypass), StateValueDefinitions.Common.CopyValueWithoutHistory,
                new StateProjectionObserver<bool>(value => runtime.OutputGainBypass = value)));
    }

    private static StateValue<TValue> CreateInstanceValue<TValue>(
        StateModelContext context,
        StatePath path,
        StateValueDefinition<TValue> definition,
        params IStateValueObserver<TValue>[] observers)
    {
        return context.Values.Create(
            StateValueCreationContext.Instance(context.InstanceId, path),
            definition,
            observers: observers);
    }

    private static StateValue<TValue> CreateBankValue<TValue>(
        StateModelContext context,
        StatePath path,
        StateValueDefinition<TValue> definition,
        params IStateValueObserver<TValue>[] observers)
    {
        return context.Values.Create(
            StateValueCreationContext.Bank(context.InstanceId, path),
            definition,
            observers: observers);
    }
}
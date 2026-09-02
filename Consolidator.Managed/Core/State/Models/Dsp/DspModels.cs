using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.State.Models.Dsp;

public sealed record CompressorState(
    StateValue<float> Attack,
    StateValue<float> Sustain,
    StateValue<float> Compression,
    StateValue<int> Character,
    StateValue<bool> Parallel,
    StateValue<float> OutputDb,
    StateValue<bool> Bypass,
    DetectorState Detector);

public sealed record DetectorState(FilterState[] Filters);

public sealed record DspState
{
    internal DspState(
        InputState inputGain,
        SaturatorState saturator,
        CompressorState compressor,
        PolishState polish,
        EqualizerState equalizer,
        EqualizerBankState[] equalizerBanks,
        ActivityObserver activity,
        OutputState outputGain)
    {
        ArgumentNullException.ThrowIfNull(inputGain);
        ArgumentNullException.ThrowIfNull(saturator);
        ArgumentNullException.ThrowIfNull(compressor);
        ArgumentNullException.ThrowIfNull(polish);
        ArgumentNullException.ThrowIfNull(equalizer);
        ArgumentNullException.ThrowIfNull(equalizerBanks);
        ArgumentNullException.ThrowIfNull(activity);
        ArgumentNullException.ThrowIfNull(outputGain);

        InputGain = inputGain;
        Saturator = saturator;
        Compressor = compressor;
        Polish = polish;
        Equalizer = equalizer;
        EqualizerBanks = equalizerBanks;
        Activity = activity;
        OutputGain = outputGain;
    }

    public InputState InputGain { get; }
    public SaturatorState Saturator { get; }
    public CompressorState Compressor { get; }
    public PolishState Polish { get; }
    public EqualizerState Equalizer { get; }
    public EqualizerBankState[] EqualizerBanks { get; }
    internal ActivityObserver Activity { get; }
    public OutputState OutputGain { get; }
}

public sealed record EqualizerBankState(
    StateValue<bool> Bypass,
    StateValue<bool> Solo,
    FilterState[] Filters);

public sealed record EqualizerState(StateValue<bool> Bypass);

public sealed record InputState(
    StateValue<float> Level,
    StateValue<float> Target,
    StateValue<float> Width,
    StateValue<bool> Leveler,
    StateValue<bool> Bypass,
    DetectorState Detector);

public sealed record LevelState(StateValue<float> Level);

public sealed record ManagedState
{
    internal ManagedState(
        InstanceState instance,
        InstanceTransientState transient,
        DspState dsp,
        ActivityObserver activity,
        DspRuntimeState runtime,
        StateNode root)
    {
        ArgumentNullException.ThrowIfNull(instance);
        ArgumentNullException.ThrowIfNull(transient);
        ArgumentNullException.ThrowIfNull(dsp);
        ArgumentNullException.ThrowIfNull(activity);
        ArgumentNullException.ThrowIfNull(runtime);
        ArgumentNullException.ThrowIfNull(root);

        Instance = instance;
        Transient = transient;
        Dsp = dsp;
        Activity = activity;
        Runtime = runtime;
        Root = root;
    }

    public InstanceState Instance { get; }
    public InstanceTransientState Transient { get; }
    public DspState Dsp { get; }
    internal ActivityObserver Activity { get; }
    public DspRuntimeState Runtime { get; }
    internal StateNode Root { get; }
}

public sealed record OutputState(
    StateValue<float> Level,
    StateValue<float> Target,
    StateValue<bool> Limiter,
    StateValue<bool> Bypass);

public sealed record PolishState(
    StateValue<float> Thick,
    StateValue<float> Air,
    StateValue<bool> Bypass);

public sealed record SaturatorState(
    StateValue<float> Drive,
    StateValue<float> Curve,
    StateValue<bool> Split,
    StateValue<float> OutputDb,
    StateValue<bool> Bypass,
    DetectorState Detector);
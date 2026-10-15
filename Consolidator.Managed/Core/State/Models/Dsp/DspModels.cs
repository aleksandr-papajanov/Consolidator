using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.State.Models.Dsp;

public sealed record ShaperState(
    StateValue<float> Attack,
    StateValue<float> Sustain,
    StateValue<float> OutputDb,
    StateValue<bool> Bypass);

public sealed record CompressionState(
    StateValue<int> Type,
    StateValue<float> Reduction,
    StateValue<float> OutputDb,
    StateValue<float> Attack,
    StateValue<float> Release,
    StateValue<float> Ratio,
    StateValue<bool> Bypass,
    StateValue<bool> Parallel,
    CompressionProfile[] Profiles,
    DetectorState Detector);

public sealed record CompressionProfile(
    StateValue<float> Reduction,
    StateValue<float> OutputDb,
    StateValue<float> Attack,
    StateValue<float> Release,
    StateValue<float> Ratio);

public sealed record DetectorState(FilterState[] Filters);

public sealed record DspState
{
    internal DspState(
        GainStagingState gainStaging,
        SaturatorState saturator,
        ShaperState shaper,
        CompressionState compression,
        PolishState polish,
        EqualizerState equalizer,
        ActivityObserver activity)
    {
        ArgumentNullException.ThrowIfNull(gainStaging);
        ArgumentNullException.ThrowIfNull(saturator);
        ArgumentNullException.ThrowIfNull(shaper);
        ArgumentNullException.ThrowIfNull(compression);
        ArgumentNullException.ThrowIfNull(polish);
        ArgumentNullException.ThrowIfNull(equalizer);
        ArgumentNullException.ThrowIfNull(activity);

        GainStaging = gainStaging;
        Saturator = saturator;
        Shaper = shaper;
        Compression = compression;
        Polish = polish;
        Equalizer = equalizer;
        Activity = activity;
    }

    public GainStagingState GainStaging { get; }
    public SaturatorState Saturator { get; }
    public ShaperState Shaper { get; }
    public CompressionState Compression { get; }
    public PolishState Polish { get; }
    public EqualizerState Equalizer { get; }
    internal ActivityObserver Activity { get; }
}

public sealed record EqualizerBankState(
    FilterState[] Filters);

public sealed record EqualizerState(StateValue<bool> Bypass);

public sealed record GainStagingState(
    StateValue<float> InputLevel,
    StateValue<float> InputTarget,
    StateValue<bool> InputLeveler,
    StateValue<float> OutputLevel,
    StateValue<float> OutputTarget,
    StateValue<bool> OutputLeveler,
    StateValue<float> Limit,
    StateValue<float> Width,
    StateValue<bool> Bypass,
    DetectorState InputDetector,
    DetectorState OutputDetector);

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
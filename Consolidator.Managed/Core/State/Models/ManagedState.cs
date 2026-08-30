using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.State.Observers;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.State.Models;

public sealed class ManagedState
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







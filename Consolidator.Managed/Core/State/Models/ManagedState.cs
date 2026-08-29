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
        ProcessorActivityObserver processorActivity,
        DspRuntimeState runtime,
        StateNode root)
    {
        ArgumentNullException.ThrowIfNull(instance);
        ArgumentNullException.ThrowIfNull(transient);
        ArgumentNullException.ThrowIfNull(dsp);
        ArgumentNullException.ThrowIfNull(processorActivity);
        ArgumentNullException.ThrowIfNull(runtime);
        ArgumentNullException.ThrowIfNull(root);

        Instance = instance;
        Transient = transient;
        Dsp = dsp;
        ProcessorActivity = processorActivity;
        Runtime = runtime;
        Root = root;
    }

    public InstanceState Instance { get; }

    public InstanceTransientState Transient { get; }

    public DspState Dsp { get; }

    internal ProcessorActivityObserver ProcessorActivity { get; }

    public DspRuntimeState Runtime { get; }

    internal StateNode Root { get; }

}







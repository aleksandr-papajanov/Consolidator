using Consolidator.Managed.Core.Dsp;
using Consolidator.Managed.Core.State;
using Consolidator.Managed.State.Tree;

namespace Consolidator.Managed.Core.State.Models;

public sealed class ManagedState
{
    public ManagedState(
        InstanceState instance,
        DspState dsp,
        DspRuntimeState runtime,
        StateNode root)
    {
        ArgumentNullException.ThrowIfNull(instance);
        ArgumentNullException.ThrowIfNull(dsp);
        ArgumentNullException.ThrowIfNull(runtime);
        ArgumentNullException.ThrowIfNull(root);

        Instance = instance;
        Dsp = dsp;
        Runtime = runtime;
        Root = root;
    }

    public InstanceState Instance { get; }

    public DspState Dsp { get; }

    public DspRuntimeState Runtime { get; }

    internal StateNode Root { get; }

}







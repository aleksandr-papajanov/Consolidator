namespace Consolidator.Managed.Core.Dsp;

public sealed class DspStateCompiler
{
    public DspSnapshot Compile(InstanceState state)
    {
        return new DspSnapshot
        {
            Gain = state.Gain
        };
    }
}
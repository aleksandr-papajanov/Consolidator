namespace Consolidator.Managed.Core.Dsp;

public sealed class DspStateCompiler
{
    public DspSnapshot Compile(DspState state)
    {
        return new DspSnapshot
        {
            Gain = state.InputGain.GainDb.Value
        };
    }
}
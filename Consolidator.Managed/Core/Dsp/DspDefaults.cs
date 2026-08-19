namespace Consolidator.Managed.Core.Dsp;

public static class DspDefaults
{
    public static InstanceState CreateState()
    {
        return new InstanceState
        {
            Gain = 1.0F
        };
    }
}
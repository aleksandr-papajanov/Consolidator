using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.Settings;

public static class DspParameterRanges
{
    public static readonly FloatRange GainDb = new(-36.0F, 36.0F);
    public static readonly FloatRange FilterGainDb = new(-24.0F, 24.0F);
    public static readonly FloatRange TargetDb = new(-36.0F, 0.0F);
    public static readonly FloatRange Width = new(0.0F, 100.0F);
    public static readonly FloatRange Macro = new(0.0F, 1.0F);
    public static readonly FloatRange Drive = Macro;
    public static readonly FloatRange Curve = Macro;
    public static readonly FloatRange OutputDb = new(-36.0F, 36.0F);
    public static readonly FloatRange FrequencyHz = new(20.0F, 20000.0F);
    public static readonly FloatRange Q = new(0.1F, 10.0F);
}




using Consolidator.Managed.Core.State;

namespace Consolidator.Managed.Core.Settings;

public static class DspParameterRanges
{
    public static readonly FloatRange GainDb = new(0.0F, 24.0F);
    public static readonly FloatRange FilterGainDb = new(-24.0F, 24.0F);
    public static readonly FloatRange Drive = new(0.0F, 24.0F);
    public static readonly FloatRange OutputDb = new(-120.0F, 24.0F);
    public static readonly FloatRange Mix = new(0.0F, 1.0F);
    public static readonly FloatRange DetectorAmount = new(0.0F, 8.0F);
    public static readonly FloatRange ThresholdDb = new(-120.0F, 0.0F);
    public static readonly FloatRange Ratio = new(1.0F, 100.0F);
    public static readonly FloatRange AttackMs = new(0.01F, 1000.0F);
    public static readonly FloatRange ReleaseMs = new(1.0F, 5000.0F);
    public static readonly FloatRange FrequencyHz = new(20.0F, 20000.0F);
    public static readonly FloatRange Q = new(0.1F, 10.0F);
}




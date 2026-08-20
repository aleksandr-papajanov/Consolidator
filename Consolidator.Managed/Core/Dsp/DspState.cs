namespace Consolidator.Managed.Core.Dsp;

public sealed class DspState
{
    public GainState InputGain { get; } = new();

    public SaturatorState Saturator { get; } = new();

    public CompressorState Compressor { get; } = new();

    public EqualizerState Equalizer { get; } = new();

    public EqualizerBankState[] EqualizerBanks { get; } =
        Enumerable.Range(0, 7)
            .Select(_ => new EqualizerBankState())
            .ToArray();

    public GainState OutputGain { get; } = new();
}

public sealed class ParameterState
{
    public ParameterState(
        ParameterId id,
        float value,
        float minimum,
        float maximum)
    {
        if (minimum > maximum)
        {
            throw new ArgumentException(
                "The minimum parameter value cannot exceed the maximum value.",
                nameof(minimum));
        }

        Id = id;
        Minimum = minimum;
        Maximum = maximum;
        Value = Math.Clamp(value, minimum, maximum);
    }

    public ParameterId Id { get; }

    public float Value { get; private set; }

    public float Minimum { get; }

    public float Maximum { get; }

    public bool SetValue(float value)
    {
        var clampedValue = Math.Clamp(value, Minimum, Maximum);
        if (Value == clampedValue)
        {
            return false;
        }

        Value = clampedValue;
        return true;
    }
}

public sealed class StateMarker
{
    public StateMarker(bool value = false)
    {
        Value = value;
    }

    public bool Value { get; private set; }

    public bool SetValue(bool value)
    {
        if (Value == value)
        {
            return false;
        }

        Value = value;
        return true;
    }
}

public sealed class GainState
{
    public ParameterState GainDb { get; } = new(ParameterId.Gain, 0.0F, -120.0F, 24.0F);

    public StateMarker Bypass { get; } = new();
}

public sealed class FilterState
{
    public ParameterState FrequencyHz { get; } = new(ParameterId.Frequency, 1000.0F, 20.0F, 20000.0F);

    public ParameterState Q { get; } = new(ParameterId.Q, 1.0F, 0.1F, 20.0F);

    public ParameterState GainDb { get; } = new(ParameterId.Gain, 0.0F, -24.0F, 24.0F);

    public StateMarker Bypass { get; } = new();

    public StateMarker Solo { get; } = new();
}

public sealed class EqualizerState
{
    public StateMarker Bypass { get; } = new();

    public StateMarker Solo { get; } = new();
}

public sealed class EqualizerBankState
{
    public StateMarker Bypass { get; } = new();

    public StateMarker Solo { get; } = new();

    public FilterState[] Filters { get; } =
        Enumerable.Range(0, 7)
            .Select(_ => new FilterState())
            .ToArray();
}

public sealed class DetectorState
{
    public FilterState[] Filters { get; } =
        Enumerable.Range(0, 2)
            .Select(_ => new FilterState())
            .ToArray();

    public StateMarker Listen { get; } = new();
}

public sealed class SaturatorState
{
    public ParameterState Drive { get; } = new(ParameterId.Drive, 0.0F, 0.0F, 24.0F);

    public ParameterState OutputDb { get; } = new(ParameterId.Gain, 0.0F, -120.0F, 24.0F);

    public ParameterState Mix { get; } = new(ParameterId.Mix, 1.0F, 0.0F, 1.0F);

    public ParameterState DetectorAmount { get; } = new(ParameterId.DetectorAmount, 1.0F, 0.0F, 8.0F);

    public StateMarker Bypass { get; } = new();

    public StateMarker Solo { get; } = new();

    public DetectorState Detector { get; } = new();
}

public sealed class CompressorState
{
    public ParameterState ThresholdDb { get; } = new(ParameterId.Threshold, -24.0F, -120.0F, 0.0F);

    public ParameterState Ratio { get; } = new(ParameterId.Ratio, 4.0F, 1.0F, 100.0F);

    public ParameterState AttackMs { get; } = new(ParameterId.Attack, 10.0F, 0.01F, 1000.0F);

    public ParameterState ReleaseMs { get; } = new(ParameterId.Release, 100.0F, 1.0F, 5000.0F);

    public ParameterState OutputDb { get; } = new(ParameterId.Gain, 0.0F, -120.0F, 24.0F);

    public ParameterState Mix { get; } = new(ParameterId.Mix, 1.0F, 0.0F, 1.0F);

    public StateMarker Bypass { get; } = new();

    public StateMarker Solo { get; } = new();

    public DetectorState Detector { get; } = new();
}
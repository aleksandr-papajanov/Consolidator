namespace Consolidator.Managed.Core.Dsp;

public unsafe sealed class DspRuntimeState
{
    private DspSnapshot _snapshot = new()
    {
        Gain = 1.0F,
        SaturatorMix = 1.0F,
        SaturatorDetectorAmount = 1.0F,
        CompressorThresholdDb = -24.0F,
        CompressorRatio = 4.0F,
        CompressorAttackMs = 10.0F,
        CompressorReleaseMs = 100.0F,
        CompressorMix = 1.0F
    };

    public DspSnapshot Snapshot => _snapshot;

    public float Gain
    {
        get => _snapshot.Gain;
        set => _snapshot.Gain = value;
    }

    public bool InputGainBypass
    {
        get => _snapshot.InputGainBypass != 0;
        set => _snapshot.InputGainBypass = value ? 1U : 0U;
    }

    public float SaturatorDrive
    {
        get => _snapshot.SaturatorDrive;
        set => _snapshot.SaturatorDrive = value;
    }

    public float SaturatorOutputDb
    {
        get => _snapshot.SaturatorOutputDb;
        set => _snapshot.SaturatorOutputDb = value;
    }

    public float SaturatorMix
    {
        get => _snapshot.SaturatorMix;
        set => _snapshot.SaturatorMix = value;
    }

    public float SaturatorDetectorAmount
    {
        get => _snapshot.SaturatorDetectorAmount;
        set => _snapshot.SaturatorDetectorAmount = value;
    }

    public bool SaturatorBypass
    {
        get => _snapshot.SaturatorBypass != 0;
        set => _snapshot.SaturatorBypass = value ? 1U : 0U;
    }

    public bool SaturatorSolo
    {
        get => _snapshot.SaturatorSolo != 0;
        set => _snapshot.SaturatorSolo = value ? 1U : 0U;
    }

    public float CompressorThresholdDb
    {
        get => _snapshot.CompressorThresholdDb;
        set => _snapshot.CompressorThresholdDb = value;
    }

    public float CompressorRatio
    {
        get => _snapshot.CompressorRatio;
        set => _snapshot.CompressorRatio = value;
    }

    public float CompressorAttackMs
    {
        get => _snapshot.CompressorAttackMs;
        set => _snapshot.CompressorAttackMs = value;
    }

    public float CompressorReleaseMs
    {
        get => _snapshot.CompressorReleaseMs;
        set => _snapshot.CompressorReleaseMs = value;
    }

    public float CompressorOutputDb
    {
        get => _snapshot.CompressorOutputDb;
        set => _snapshot.CompressorOutputDb = value;
    }

    public float CompressorMix
    {
        get => _snapshot.CompressorMix;
        set => _snapshot.CompressorMix = value;
    }

    public bool CompressorBypass
    {
        get => _snapshot.CompressorBypass != 0;
        set => _snapshot.CompressorBypass = value ? 1U : 0U;
    }

    public bool CompressorSolo
    {
        get => _snapshot.CompressorSolo != 0;
        set => _snapshot.CompressorSolo = value ? 1U : 0U;
    }

    public bool EqualizerBypass
    {
        get => _snapshot.EqualizerBypass != 0;
        set => _snapshot.EqualizerBypass = value ? 1U : 0U;
    }

    public bool EqualizerSolo
    {
        get => _snapshot.EqualizerSolo != 0;
        set => _snapshot.EqualizerSolo = value ? 1U : 0U;
    }

    public float OutputGain
    {
        get => _snapshot.OutputGain;
        set => _snapshot.OutputGain = value;
    }

    public bool OutputGainBypass
    {
        get => _snapshot.OutputGainBypass != 0;
        set => _snapshot.OutputGainBypass = value ? 1U : 0U;
    }

    public bool InputGainActive
    {
        get => _snapshot.InputGainActive != 0;
        set => _snapshot.InputGainActive = value ? 1U : 0U;
    }

    public bool SaturatorActive
    {
        get => _snapshot.SaturatorActive != 0;
        set => _snapshot.SaturatorActive = value ? 1U : 0U;
    }

    public bool CompressorActive
    {
        get => _snapshot.CompressorActive != 0;
        set => _snapshot.CompressorActive = value ? 1U : 0U;
    }

    public bool EqualizerActive
    {
        get => _snapshot.EqualizerActive != 0;
        set => _snapshot.EqualizerActive = value ? 1U : 0U;
    }

    public bool OutputGainActive
    {
        get => _snapshot.OutputGainActive != 0;
        set => _snapshot.OutputGainActive = value ? 1U : 0U;
    }

    public bool SaturatorListen
    {
        get => _snapshot.SaturatorListen != 0;
        set => _snapshot.SaturatorListen = value ? 1U : 0U;
    }

    public bool CompressorListen
    {
        get => _snapshot.CompressorListen != 0;
        set => _snapshot.CompressorListen = value ? 1U : 0U;
    }

    public bool Audible
    {
        get => _snapshot.Audible != 0;
        set => _snapshot.Audible = value ? 1U : 0U;
    }

    public void SetEqualizerBankActive(int index, bool active)
    {
        if ((uint)index >= 7)
        {
            throw new ArgumentOutOfRangeException(nameof(index));
        }

        _snapshot.EqualizerBanksActive[index] = active ? 1U : 0U;
    }

    public void SetEqualizerFilterActive(int bankIndex, int filterIndex, bool active)
    {
        if ((uint)bankIndex >= 7)
        {
            throw new ArgumentOutOfRangeException(nameof(bankIndex));
        }

        if ((uint)filterIndex >= 7)
        {
            throw new ArgumentOutOfRangeException(nameof(filterIndex));
        }

        _snapshot.EqualizerFiltersActive[(bankIndex * 7) + filterIndex] = active ? 1U : 0U;
    }

    public void SetDetectorFilterActive(int index, bool active)
    {
        if ((uint)index >= 4)
        {
            throw new ArgumentOutOfRangeException(nameof(index));
        }

        _snapshot.DetectorFiltersActive[index] = active ? 1U : 0U;
    }
}





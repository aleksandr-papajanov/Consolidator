namespace Consolidator.Managed.Core.Dsp;

public unsafe sealed class DspRuntimeState
{
    private DspSnapshot _snapshot = new()
    {
        InputLevel = 0.0F,
        InputTarget = -18.0F,
        InputWidth = 100.0F,
        SaturatorCurve = 0.5F,
        CompressorAttack = 0.5F,
        CompressorSustain = 0.5F,
        CompressorCompression = 0.5F,
        OutputTarget = -1.0F
    };

    public DspSnapshot Snapshot => _snapshot;

    public float InputLevel
    {
        get => _snapshot.InputLevel;
        set => _snapshot.InputLevel = value;
    }

    public float InputTarget { get => _snapshot.InputTarget; set => _snapshot.InputTarget = value; }
    public float InputWidth { get => _snapshot.InputWidth; set => _snapshot.InputWidth = value; }
    public bool InputLeveler { get => _snapshot.InputLeveler != 0; set => _snapshot.InputLeveler = value ? 1U : 0U; }

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

    public float SaturatorCurve
    {
        get => _snapshot.SaturatorCurve;
        set => _snapshot.SaturatorCurve = value;
    }
    public bool SaturatorSplit { get => _snapshot.SaturatorSplit != 0; set => _snapshot.SaturatorSplit = value ? 1U : 0U; }

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

    public float CompressorAttack
    {
        get => _snapshot.CompressorAttack;
        set => _snapshot.CompressorAttack = value;
    }
    public float CompressorSustain { get => _snapshot.CompressorSustain; set => _snapshot.CompressorSustain = value; }
    public float CompressorCompression { get => _snapshot.CompressorCompression; set => _snapshot.CompressorCompression = value; }
    public int CompressorCharacter { get => (int)_snapshot.CompressorCharacter; set => _snapshot.CompressorCharacter = (uint)value; }
    public bool CompressorParallel { get => _snapshot.CompressorParallel != 0; set => _snapshot.CompressorParallel = value ? 1U : 0U; }

    public float CompressorOutputDb
    {
        get => _snapshot.CompressorOutputDb;
        set => _snapshot.CompressorOutputDb = value;
    }

    public float PolishThick { get => _snapshot.PolishThick; set => _snapshot.PolishThick = value; }
    public float PolishAir { get => _snapshot.PolishAir; set => _snapshot.PolishAir = value; }
    public bool PolishBypass { get => _snapshot.PolishBypass != 0; set => _snapshot.PolishBypass = value ? 1U : 0U; }
    public bool PolishSolo { get => _snapshot.PolishSolo != 0; set => _snapshot.PolishSolo = value ? 1U : 0U; }

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

    public float OutputLevel
    {
        get => _snapshot.OutputLevel;
        set => _snapshot.OutputLevel = value;
    }
    public float OutputTarget { get => _snapshot.OutputTarget; set => _snapshot.OutputTarget = value; }
    public bool OutputLimiter { get => _snapshot.OutputLimiter != 0; set => _snapshot.OutputLimiter = value ? 1U : 0U; }

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

    public bool InputListen
    {
        get => _snapshot.InputListen != 0;
        set => _snapshot.InputListen = value ? 1U : 0U;
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
        if ((uint)index >= 6)
        {
            throw new ArgumentOutOfRangeException(nameof(index));
        }

        _snapshot.DetectorFiltersActive[index] = active ? 1U : 0U;
    }
}





using System.Runtime.InteropServices;

namespace Consolidator.Managed.Core.Dsp;

[StructLayout(LayoutKind.Sequential)]
public unsafe struct DspSnapshot
{
    public float InputLevel;
    public float InputTarget;
    public float InputWidth;

    public uint InputGainBypass;
    public uint InputLeveler;

    public float SaturatorDrive;
    public float SaturatorOutputDb;
    public float SaturatorCurve;
    public uint SaturatorSplit;
    public uint SaturatorBypass;

    public float CompressorAttack;
    public float CompressorSustain;
    public float CompressorCompression;
    public uint CompressorCharacter;
    public uint CompressorParallel;
    public float CompressorOutputDb;
    public uint CompressorBypass;

    public uint EqualizerBypass;

    public float PolishThick;
    public float PolishAir;
    public uint PolishBypass;

    public float OutputLevel;
    public float OutputTarget;
    public uint OutputGainBypass;
    public uint OutputLimiter;

    public uint Audible;
    public uint InputGainActive;
    public uint SaturatorActive;
    public uint CompressorActive;
    public uint EqualizerActive;
    public uint OutputGainActive;
    public fixed uint EqualizerBanksActive[7];

    public fixed uint EqualizerFiltersActive[49];

    public fixed uint DetectorFiltersActive[6];
}




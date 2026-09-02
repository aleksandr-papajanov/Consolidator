using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace Consolidator.Managed.Core.Dsp;

[StructLayout(LayoutKind.Sequential)]
public struct FilterSnapshot
{
    public uint Active;
    public uint Type;
    public float FrequencyHz;
    public float GainDb;
    public float Q;
    public float FixedQ;
}

[InlineArray(49)]
public struct EqualizerFilterSnapshotBuffer
{
    private FilterSnapshot _element;
}

[InlineArray(6)]
public struct DetectorFilterSnapshotBuffer
{
    private FilterSnapshot _element;
}

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
    public uint InstanceBypass;
    public uint InputGainActive;
    public uint SaturatorActive;
    public uint CompressorActive;
    public uint EqualizerActive;
    public uint OutputGainActive;
    public fixed uint EqualizerBanksActive[7];

    public EqualizerFilterSnapshotBuffer EqualizerFilters;

    public DetectorFilterSnapshotBuffer DetectorFilters;
}




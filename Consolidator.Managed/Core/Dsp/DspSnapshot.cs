using System.Runtime.InteropServices;

namespace Consolidator.Managed.Core.Dsp;

[StructLayout(LayoutKind.Sequential)]
public unsafe struct DspSnapshot
{
    public float Gain;

    public uint InputGainBypass;

    public float SaturatorDrive;
    public float SaturatorOutputDb;
    public float SaturatorMix;
    public float SaturatorDetectorAmount;
    public uint SaturatorBypass;
    public uint SaturatorSolo;

    public float CompressorThresholdDb;
    public float CompressorRatio;
    public float CompressorAttackMs;
    public float CompressorReleaseMs;
    public float CompressorOutputDb;
    public float CompressorMix;
    public uint CompressorBypass;
    public uint CompressorSolo;

    public uint EqualizerBypass;
    public uint EqualizerSolo;

    public float OutputGain;
    public uint OutputGainBypass;

    public uint Audible;
    public uint InputGainActive;
    public uint SaturatorActive;
    public uint CompressorActive;
    public uint EqualizerActive;
    public uint OutputGainActive;
    public uint SaturatorListen;
    public uint CompressorListen;

    public fixed uint EqualizerBanksActive[7];

    public fixed uint EqualizerFiltersActive[49];

    public fixed uint DetectorFiltersActive[4];
}




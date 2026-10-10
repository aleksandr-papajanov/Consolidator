param(
    [Parameter(Mandatory = $true)][string]$InputPath,
    [Parameter(Mandatory = $true)][string]$OutputPath,
    [double]$LowHz = 120.0,
    [double]$HighHz = 500.0
)

$ErrorActionPreference = 'Stop'
$source = [System.IO.Path]::GetFullPath($InputPath)
$target = [System.IO.Path]::GetFullPath($OutputPath)
if ($source -eq $target -or [System.IO.File]::Exists($target))
{
    throw 'Output must be a new WAV path distinct from the input.'
}

Add-Type -TypeDefinition @'
using System;
using System.IO;

public static class OfflineBandpass
{
    private sealed class Biquad
    {
        private readonly double b0;
        private readonly double b1;
        private readonly double b2;
        private readonly double a1;
        private readonly double a2;
        private double z1;
        private double z2;

        public Biquad(double cutoff, double sampleRate, bool highpass)
        {
            double angle = 2.0 * Math.PI * cutoff / sampleRate;
            double cosine = Math.Cos(angle);
            double alpha = Math.Sin(angle) / (2.0 * Math.Sqrt(0.5));
            double a0 = 1.0 + alpha;
            a1 = -2.0 * cosine / a0;
            a2 = (1.0 - alpha) / a0;
            if (highpass)
            {
                b0 = (1.0 + cosine) * 0.5 / a0;
                b1 = -(1.0 + cosine) / a0;
                b2 = b0;
            }
            else
            {
                b0 = (1.0 - cosine) * 0.5 / a0;
                b1 = (1.0 - cosine) / a0;
                b2 = b0;
            }
        }

        public double Process(double input)
        {
            double output = b0 * input + z1;
            z1 = b1 * input - a1 * output + z2;
            z2 = b2 * input - a2 * output;
            return output;
        }
    }

    public static void Render(string source, string target, double lowHz, double highHz)
    {
        using (var input = new BinaryReader(File.OpenRead(source)))
        {
            if (new string(input.ReadChars(4)) != "RIFF")
                throw new InvalidDataException("RIFF expected.");
            input.ReadUInt32();
            if (new string(input.ReadChars(4)) != "WAVE")
                throw new InvalidDataException("WAVE expected.");
            int format = 0;
            int channels = 0;
            int sampleRate = 0;
            int bits = 0;
            long dataOffset = 0;
            int dataBytes = 0;
            while (input.BaseStream.Position + 8 <= input.BaseStream.Length)
            {
                string chunk = new string(input.ReadChars(4));
                int size = input.ReadInt32();
                long next = input.BaseStream.Position + size + (size & 1);
                if (chunk == "fmt ")
                {
                    format = input.ReadUInt16();
                    channels = input.ReadUInt16();
                    sampleRate = input.ReadInt32();
                    input.ReadInt32();
                    input.ReadUInt16();
                    bits = input.ReadUInt16();
                }
                if (chunk == "data")
                {
                    dataOffset = input.BaseStream.Position;
                    dataBytes = size;
                    break;
                }
                input.BaseStream.Position = next;
            }
            if (dataOffset == 0 || channels != 2 ||
                !((format == 1 && bits == 24) || (format == 3 && bits == 32)))
                throw new InvalidDataException("Expected stereo PCM24 or float32 WAV.");
            if (lowHz <= 0 || lowHz >= highHz || highHz >= sampleRate * 0.45)
                throw new ArgumentOutOfRangeException("lowHz/highHz");
            int frames = dataBytes / (channels * bits / 8);
            int outputBytes = checked(frames * 8);
            input.BaseStream.Position = dataOffset;

            using (var output = new BinaryWriter(File.Create(target)))
            {
                output.Write("RIFF".ToCharArray());
                output.Write(checked(36 + outputBytes));
                output.Write("WAVE".ToCharArray());
                output.Write("fmt ".ToCharArray());
                output.Write(16);
                output.Write((short)3);
                output.Write((short)2);
                output.Write(sampleRate);
                output.Write(checked(sampleRate * 8));
                output.Write((short)8);
                output.Write((short)32);
                output.Write("data".ToCharArray());
                output.Write(outputBytes);

                var highpass = new[] {
                    new Biquad(lowHz, sampleRate, true),
                    new Biquad(lowHz, sampleRate, true) };
                var lowpass = new[] {
                    new Biquad(highHz, sampleRate, false),
                    new Biquad(highHz, sampleRate, false) };
                for (int frame = 0; frame < frames; ++frame)
                {
                    for (int channel = 0; channel < 2; ++channel)
                    {
                        double sample;
                        if (format == 3) sample = input.ReadSingle();
                        else
                        {
                            int value = input.ReadByte() | (input.ReadByte() << 8) |
                                (input.ReadByte() << 16);
                            if ((value & 0x800000) != 0)
                                value |= unchecked((int)0xFF000000);
                            sample = value / 8388608.0;
                        }
                        double filtered = lowpass[channel].Process(
                            highpass[channel].Process(sample));
                        output.Write((float)filtered);
                    }
                }
            }
        }
    }
}
'@

[OfflineBandpass]::Render($source, $target, $LowHz, $HighHz)
Write-Output "Rendered stereo bandpass WAV: $target"

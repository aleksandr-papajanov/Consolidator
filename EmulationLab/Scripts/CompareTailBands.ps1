param(
    [Parameter(Mandatory = $true)][string]$DryPath,
    [Parameter(Mandatory = $true)][string]$ReferencePath,
    [Parameter(Mandatory = $true)][string]$CandidatePath,
    [Parameter(Mandatory = $true)][int[]]$Onsets,
    [int[]]$WindowEdgesMs = @(10, 50, 100, 200, 400)
)

$ErrorActionPreference = 'Stop'

Add-Type -ReferencedAssemblies 'System.Runtime.Numerics.dll' -TypeDefinition @'
using System;
using System.IO;
using System.Numerics;

public static class TailBandAnalysis
{
    public static double[] ReadMono(string path, out int sampleRate)
    {
        using (var reader = new BinaryReader(File.OpenRead(path)))
        {
            if (new string(reader.ReadChars(4)) != "RIFF") throw new InvalidDataException("RIFF expected");
            reader.ReadUInt32();
            if (new string(reader.ReadChars(4)) != "WAVE") throw new InvalidDataException("WAVE expected");

            int format = 0;
            int channels = 0;
            int bits = 0;
            sampleRate = 0;
            long dataStart = 0;
            int dataBytes = 0;
            while (reader.BaseStream.Position + 8 <= reader.BaseStream.Length)
            {
                string chunk = new string(reader.ReadChars(4));
                int size = reader.ReadInt32();
                long next = reader.BaseStream.Position + size + (size & 1);
                if (chunk == "fmt ")
                {
                    format = reader.ReadUInt16();
                    channels = reader.ReadUInt16();
                    sampleRate = reader.ReadInt32();
                    reader.ReadInt32();
                    reader.ReadUInt16();
                    bits = reader.ReadUInt16();
                }
                if (chunk == "data")
                {
                    dataStart = reader.BaseStream.Position;
                    dataBytes = size;
                    break;
                }
                reader.BaseStream.Position = next;
            }

            if (dataStart == 0 || channels != 2 ||
                !((format == 1 && bits == 24) || (format == 3 && bits == 32)))
                throw new InvalidDataException("Expected stereo PCM24 or float32 WAV: " + path);

            reader.BaseStream.Position = dataStart;
            int frames = dataBytes / (channels * bits / 8);
            var mono = new double[frames];
            for (int frame = 0; frame < frames; ++frame)
            {
                double sum = 0;
                for (int channel = 0; channel < channels; ++channel)
                {
                    if (format == 3) sum += reader.ReadSingle();
                    else
                    {
                        int value = reader.ReadByte() | (reader.ReadByte() << 8) |
                            (reader.ReadByte() << 16);
                        if ((value & 0x800000) != 0) value |= unchecked((int)0xFF000000);
                        sum += value / 8388608.0;
                    }
                }
                mono[frame] = sum / channels;
            }
            return mono;
        }
    }

    public static double[] BandPower(double[] audio, int rate, int onset, int startMs, int endMs)
    {
        int start = onset + (int)Math.Round(startMs * rate / 1000.0);
        int length = (int)Math.Round((endMs - startMs) * rate / 1000.0);
        if (start < 0 || start + length > audio.Length)
            throw new ArgumentOutOfRangeException("onset");
        int count = 1;
        while (count < length) count <<= 1;
        var spectrum = new Complex[count];
        for (int index = 0; index < length; ++index)
        {
            double hann = 0.5 - 0.5 * Math.Cos(2.0 * Math.PI * index / (length - 1));
            spectrum[index] = audio[start + index] * hann;
        }

        for (int index = 1, reverse = 0; index < count; ++index)
        {
            int bit = count >> 1;
            for (; (reverse & bit) != 0; bit >>= 1) reverse ^= bit;
            reverse ^= bit;
            if (index < reverse)
            {
                Complex value = spectrum[index];
                spectrum[index] = spectrum[reverse];
                spectrum[reverse] = value;
            }
        }
        for (int lengthFft = 2; lengthFft <= count; lengthFft <<= 1)
        {
            double angle = -2.0 * Math.PI / lengthFft;
            var rotation = new Complex(Math.Cos(angle), Math.Sin(angle));
            for (int startFft = 0; startFft < count; startFft += lengthFft)
            {
                Complex factor = Complex.One;
                for (int offset = 0; offset < lengthFft / 2; ++offset)
                {
                    Complex first = spectrum[startFft + offset];
                    Complex second = spectrum[startFft + offset + lengthFft / 2] * factor;
                    spectrum[startFft + offset] = first + second;
                    spectrum[startFft + offset + lengthFft / 2] = first - second;
                    factor *= rotation;
                }
            }
        }

        double[] edges = { 0, 120, 500, 2000, 8000, rate / 2.0 + 1 };
        var power = new double[5];
        for (int bin = 0; bin <= count / 2; ++bin)
        {
            double frequency = (double)bin * rate / count;
            for (int band = 0; band < power.Length; ++band)
            {
                if (frequency >= edges[band] && frequency < edges[band + 1])
                {
                    double magnitude = spectrum[bin].Magnitude;
                    power[band] += magnitude * magnitude;
                    break;
                }
            }
        }
        return power;
    }
}
'@

$rate = 0
$dry = [TailBandAnalysis]::ReadMono($DryPath, [ref]$rate)
$referenceRate = 0
$reference = [TailBandAnalysis]::ReadMono($ReferencePath, [ref]$referenceRate)
$candidateRate = 0
$candidate = [TailBandAnalysis]::ReadMono($CandidatePath, [ref]$candidateRate)
if ($rate -ne $referenceRate -or $rate -ne $candidateRate -or
    $dry.Length -ne $reference.Length -or $dry.Length -ne $candidate.Length)
{
    throw 'WAV rates and lengths must match.'
}

for ($windowIndex = 0; $windowIndex -lt $WindowEdgesMs.Length - 1; ++$windowIndex)
{
    $window = @($WindowEdgesMs[$windowIndex], $WindowEdgesMs[$windowIndex + 1])
    $referencePower = [double[]]::new(5)
    $candidatePower = [double[]]::new(5)
    foreach ($onset in $Onsets)
    {
        $referenceBands = [TailBandAnalysis]::BandPower($reference, $rate, $onset, $window[0], $window[1])
        $candidateBands = [TailBandAnalysis]::BandPower($candidate, $rate, $onset, $window[0], $window[1])
        for ($band = 0; $band -lt 5; ++$band)
        {
            $referencePower[$band] += $referenceBands[$band]
            $candidatePower[$band] += $candidateBands[$band]
        }
    }
    $difference = @(0..4 | ForEach-Object {
        [math]::Round(10 * [math]::Log10(
            [math]::Max(1e-20, $candidatePower[$_]) /
            [math]::Max(1e-20, $referencePower[$_])), 1)
    })
    [pscustomobject]@{
        WindowMs = "$($window[0])-$($window[1])"
        Below120Hz = $difference[0]
        From120To500Hz = $difference[1]
        From500To2000Hz = $difference[2]
        From2000To8000Hz = $difference[3]
        Above8000Hz = $difference[4]
    }
}

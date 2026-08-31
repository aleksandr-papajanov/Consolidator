using System.Text.Json.Serialization;

namespace Consolidator.Managed.Core.Services.Persistence;

internal sealed record PersistentStateV1(
    int Schema,
    PersistentInstance Instance,
    PersistentBank[] Banks,
    PersistentDsp Dsp);

internal sealed record PersistentInstance(bool Mute, bool Solo);

internal sealed record PersistentBank(uint? Group);

internal sealed record PersistentDsp(
    PersistentGain Input,
    PersistentSaturator Saturator,
    PersistentCompressor Compressor,
    PersistentEqualizer Equalizer,
    PersistentGain Output);

internal sealed record PersistentGain(float GainDb, bool Bypass);

internal sealed record PersistentSaturator(
    float Drive,
    float OutputDb,
    float Mix,
    float DetectorAmount,
    bool Bypass,
    bool Solo,
    PersistentDetector Detector);

internal sealed record PersistentCompressor(
    float ThresholdDb,
    float Ratio,
    float AttackMs,
    float ReleaseMs,
    float OutputDb,
    float Mix,
    bool Bypass,
    bool Solo,
    PersistentDetector Detector);

internal sealed record PersistentDetector(
    bool Listen,
    PersistentFilter[] Filters);

internal sealed record PersistentEqualizer(
    bool Bypass,
    bool Solo,
    PersistentEqualizerBank[] Banks);

internal sealed record PersistentEqualizerBank(
    bool Bypass,
    bool Solo,
    PersistentFilter[] Filters);

internal sealed record PersistentFilter(
    float? FrequencyHz,
    float? Q,
    float GainDb,
    bool Bypass,
    bool Solo);

[JsonSourceGenerationOptions(
    PropertyNamingPolicy = JsonKnownNamingPolicy.CamelCase,
    NumberHandling = JsonNumberHandling.Strict,
    RespectRequiredConstructorParameters = true,
    UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow)]
[JsonSerializable(typeof(PersistentStateV1))]
internal partial class PersistenceJsonContext : JsonSerializerContext
{
}

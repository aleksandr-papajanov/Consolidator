using System.Text.Json.Serialization;

namespace Consolidator.Managed.Core.Services.Persistence;

internal sealed record PersistentState(
    int Schema,
    PersistentInstance Instance,
    PersistentBank[] Banks,
    PersistentDsp Dsp);

internal sealed record PersistentInstance(bool Mute, bool Solo, bool Bypass);

internal sealed record PersistentBank(uint? Group);

internal sealed record PersistentDsp(
    PersistentGainStaging GainStaging,
    PersistentSaturator Saturator,
    PersistentCompressor Compressor,
    PersistentEqualizer Equalizer,
    PersistentPolish Polish);

internal sealed record PersistentGainStaging(
    float InputLevel, float InputTarget, bool InputLeveler,
    float OutputLevel, float OutputTarget, bool OutputLeveler,
    float Limit, float Width, bool Bypass, PersistentDetector Detector);

internal sealed record PersistentSaturator(
    float Drive,
    float Curve,
    bool Split,
    float Output,
    bool Bypass,
    PersistentDetector Detector);

internal sealed record PersistentCompressor(
    float Attack,
    float Sustain,
    float Compression,
    int Character,
    bool Parallel,
    float Output,
    bool Bypass,
    PersistentDetector Detector);

internal sealed record PersistentPolish(float Thick, float Air, bool Bypass);

internal sealed record PersistentDetector(
    PersistentFilter[] Filters);

internal sealed record PersistentEqualizer(
    bool Bypass,
    PersistentEqualizerBank[] Banks);

internal sealed record PersistentEqualizerBank(
    bool Bypass,
    PersistentFilter[] Filters);

internal sealed record PersistentFilter(
    float? FrequencyHz,
    float? Q,
    float GainDb,
    bool Bypass);

[JsonSourceGenerationOptions(
    PropertyNamingPolicy = JsonKnownNamingPolicy.CamelCase,
    NumberHandling = JsonNumberHandling.Strict,
    RespectRequiredConstructorParameters = true,
    UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow)]
[JsonSerializable(typeof(PersistentState))]
internal partial class PersistenceJsonContext : JsonSerializerContext
{
}

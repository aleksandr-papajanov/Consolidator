using System.Text.Json.Serialization;

namespace Consolidator.Managed.Core.Services.Persistence;

internal sealed record PersistentStateV4(
    int Schema,
    PersistentInstance Instance,
    PersistentBank[] Banks,
    PersistentDsp Dsp);

internal sealed record PersistentInstance(bool Mute, bool Solo, bool Bypass);

internal sealed record PersistentBank(uint? Group);

internal sealed record PersistentDsp(
    PersistentInput Input,
    PersistentSaturator Saturator,
    PersistentCompressor Compressor,
    PersistentEqualizer Equalizer,
    PersistentPolish Polish,
    PersistentOutput Output);

internal sealed record PersistentInput(float Level, float Target, float Width, bool Leveler, bool Bypass);
internal sealed record PersistentOutput(float Level, float Target, bool Limiter, bool Bypass);

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
[JsonSerializable(typeof(PersistentStateV4))]
internal partial class PersistenceJsonContext : JsonSerializerContext
{
}

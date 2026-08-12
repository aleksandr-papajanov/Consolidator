#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace consolidator::dsp
{

enum class MeterPoint : std::uint8_t
{
    InputGainOutput,
    SaturatorOutput,
    CompressorOutput,
    OutputGainOutput,
    Count
};

constexpr std::size_t ToIndex(MeterPoint point) noexcept
{
    return static_cast<std::size_t>(point);
}

struct LevelTelemetry
{
    float rmsDb = -240.0f;
    float peakDb = -240.0f;
    float smoothedDb = -240.0f;
};

struct CompressorTelemetry
{
    float gainReductionRmsDb = 0.0f;
    float gainReductionPeakDb = 0.0f;
    float gainReductionSmoothedDb = 0.0f;
};

struct SaturatorTelemetry
{
    float distortionPercent = 0.0f;
    float distortionSmoothedPercent = 0.0f;
};

struct TelemetrySnapshot
{
    std::array<LevelTelemetry, ToIndex(MeterPoint::Count)> levels{};
    CompressorTelemetry compressor;
    SaturatorTelemetry saturator;
    std::uint64_t revision = 0;
    std::uint64_t viewRevision = 0;
};

} // namespace consolidator::dsp

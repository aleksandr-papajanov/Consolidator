#pragma once

#include <cstddef>
#include <cstdint>

namespace consolidator::max::dsp
{

struct FilterSnapshot
{
    std::uint32_t active;
    std::uint32_t type;
    float frequencyHz;
    float gainDb;
    float q;
    float fixedQ;
};

struct DspSnapshot
{
    float inputLevel;
    float inputTarget;
    float inputWidth;
    std::uint32_t inputGainBypass;
    std::uint32_t inputLeveler;
    float saturatorDrive;
    float saturatorOutputDb;
    float saturatorCurve;
    std::uint32_t saturatorSplit;
    std::uint32_t saturatorBypass;
    float compressorAttack;
    float compressorSustain;
    float compressorCompression;
    std::uint32_t compressorCharacter;
    std::uint32_t compressorParallel;
    float compressorOutputDb;
    std::uint32_t compressorBypass;
    std::uint32_t equalizerBypass;
    float polishThick;
    float polishAir;
    std::uint32_t polishBypass;
    float outputLevel;
    float outputTarget;
    std::uint32_t outputGainBypass;
    std::uint32_t outputLimiter;
    std::uint32_t audible;
    std::uint32_t instanceBypass;
    std::uint32_t inputGainActive;
    std::uint32_t saturatorActive;
    std::uint32_t compressorActive;
    std::uint32_t equalizerActive;
    std::uint32_t outputGainActive;
    std::uint32_t equalizerBanksActive[7];
    FilterSnapshot equalizerFilters[49];
    FilterSnapshot detectorFilters[6];
};

struct DspTelemetryExchange
{
    std::uint32_t sequence{};
    std::uint32_t valuesPresent{};
    std::uint32_t inputLevelBits{};
    std::uint32_t inputLevelFastBits{};
    std::uint32_t outputLevelBits{};
};

enum class DspTelemetryValues : std::uint32_t
{
    None = 0,
    InputLevel = 1U << 0,
    InputLevelFast = 1U << 1,
    OutputLevel = 1U << 2
};

struct DspStateExchange
{
    DspSnapshot snapshots[3];
    std::uint32_t publishedIndex{};
    std::uint32_t consumerIndex{};
    DspTelemetryExchange telemetry{};
};

static_assert(sizeof(FilterSnapshot) == 24);
static_assert(sizeof(DspSnapshot) == 1476);
static_assert(sizeof(DspTelemetryExchange) == 20);
static_assert(sizeof(DspStateExchange) == 4456);
static_assert(offsetof(DspStateExchange, snapshots) == 0);
static_assert(offsetof(DspStateExchange, publishedIndex) == 4428);
static_assert(offsetof(DspStateExchange, consumerIndex) == 4432);
static_assert(offsetof(DspStateExchange, telemetry) == 4436);

} // namespace consolidator::max::dsp

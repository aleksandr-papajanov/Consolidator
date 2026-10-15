#pragma once

#include <cstddef>
#include <cstdint>

namespace consolidator::transport
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
    std::uint32_t outputLeveler;
    float outputLimit;
    float dynamicsCompressionOutputDb;
    float dynamicsCompressionAttack;
    float dynamicsCompressionRelease;
    float dynamicsCompressionRatio;
    std::uint32_t dynamicsShaperBypass;
    std::uint32_t dynamicsCompressionBypass;
    FilterSnapshot outputDetectorFilters[2];
};

struct DspTelemetryExchange
{
    std::uint32_t sequence{};
    std::uint32_t valuesPresent{};
    std::uint32_t inputLevelBits{};
    std::uint32_t inputLevelFastBits{};
    std::uint32_t outputLevelBits{};
    std::uint32_t inputRmsBits{};
    std::uint32_t outputGainBits{};
    std::uint32_t outputGainFastBits{};
    std::uint32_t outputRmsBits{};
    std::uint32_t compressorReductionBits{};
    std::uint32_t limiterReductionBits{};
};

enum class DspTelemetryValues : std::uint32_t
{
    None = 0,
    InputLevel = 1U << 0,
    InputLevelFast = 1U << 1,
    OutputLevel = 1U << 2,
    InputRms = 1U << 3,
    OutputGain = 1U << 4,
    OutputGainFast = 1U << 5,
    OutputRms = 1U << 6,
    CompressorReduction = 1U << 7,
    LimiterReduction = 1U << 8
};

struct DspStateExchange
{
    DspSnapshot snapshots[3];
    std::uint32_t publishedIndex{};
    std::uint32_t consumerIndex{};
    DspTelemetryExchange telemetry{};
};

static_assert(sizeof(FilterSnapshot) == 24);
static_assert(sizeof(DspSnapshot) == 1556);
static_assert(offsetof(DspSnapshot, outputDetectorFilters) == 1508);
static_assert(sizeof(DspTelemetryExchange) == 44);
static_assert(sizeof(DspStateExchange) == 4720);
static_assert(offsetof(DspStateExchange, snapshots) == 0);
static_assert(offsetof(DspStateExchange, publishedIndex) == 4668);
static_assert(offsetof(DspStateExchange, consumerIndex) == 4672);
static_assert(offsetof(DspStateExchange, telemetry) == 4676);

} // namespace consolidator::transport

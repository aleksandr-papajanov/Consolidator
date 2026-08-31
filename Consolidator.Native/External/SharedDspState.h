#pragma once

#include <cstddef>
#include <cstdint>

namespace consolidator::max
{

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
    std::uint32_t saturatorSolo;
    float compressorAttack;
    float compressorSustain;
    float compressorCompression;
    std::uint32_t compressorCharacter;
    std::uint32_t compressorParallel;
    float compressorOutputDb;
    std::uint32_t compressorBypass;
    std::uint32_t compressorSolo;
    std::uint32_t equalizerBypass;
    std::uint32_t equalizerSolo;
    float polishThick;
    float polishAir;
    std::uint32_t polishBypass;
    std::uint32_t polishSolo;
    float outputLevel;
    float outputTarget;
    std::uint32_t outputGainBypass;
    std::uint32_t outputLimiter;
    std::uint32_t audible;
    std::uint32_t inputGainActive;
    std::uint32_t saturatorActive;
    std::uint32_t compressorActive;
    std::uint32_t equalizerActive;
    std::uint32_t outputGainActive;
    std::uint32_t inputListen;
    std::uint32_t saturatorListen;
    std::uint32_t compressorListen;
    std::uint32_t equalizerBanksActive[7];
    std::uint32_t equalizerFiltersActive[49];
    std::uint32_t detectorFiltersActive[6];
};

struct SharedDspExchange
{
    DspSnapshot snapshots[3];
    std::uint32_t publishedIndex{};
    std::uint32_t consumerIndex{};
};

static_assert(sizeof(DspSnapshot) == 400);
static_assert(sizeof(SharedDspExchange) == 1208);
static_assert(offsetof(SharedDspExchange, snapshots) == 0);
static_assert(offsetof(SharedDspExchange, publishedIndex) == 1200);
static_assert(offsetof(SharedDspExchange, consumerIndex) == 1204);

} // namespace consolidator::max

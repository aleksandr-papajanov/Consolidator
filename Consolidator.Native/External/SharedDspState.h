#pragma once

#include <cstddef>
#include <cstdint>

namespace consolidator::max
{

struct DspSnapshot
{
    float gain;
    std::uint32_t inputGainBypass;
    float saturatorDrive;
    float saturatorOutputDb;
    float saturatorMix;
    float saturatorDetectorAmount;
    std::uint32_t saturatorBypass;
    std::uint32_t saturatorSolo;
    float compressorThresholdDb;
    float compressorRatio;
    float compressorAttackMs;
    float compressorReleaseMs;
    float compressorOutputDb;
    float compressorMix;
    std::uint32_t compressorBypass;
    std::uint32_t compressorSolo;
    std::uint32_t equalizerBypass;
    std::uint32_t equalizerSolo;
    float outputGain;
    std::uint32_t outputGainBypass;
    std::uint32_t audible;
    std::uint32_t inputGainActive;
    std::uint32_t saturatorActive;
    std::uint32_t compressorActive;
    std::uint32_t equalizerActive;
    std::uint32_t outputGainActive;
    std::uint32_t saturatorListen;
    std::uint32_t compressorListen;
    std::uint32_t equalizerBanksActive[7];
    std::uint32_t equalizerFiltersActive[49];
    std::uint32_t detectorFiltersActive[4];
};

struct SharedDspExchange
{
    DspSnapshot snapshots[3];
    std::uint32_t publishedIndex{};
    std::uint32_t consumerIndex{};
};

static_assert(sizeof(DspSnapshot) == 352);
static_assert(sizeof(SharedDspExchange) == 1064);
static_assert(offsetof(SharedDspExchange, snapshots) == 0);
static_assert(offsetof(SharedDspExchange, publishedIndex) == 1056);
static_assert(offsetof(SharedDspExchange, consumerIndex) == 1060);

} // namespace consolidator::max
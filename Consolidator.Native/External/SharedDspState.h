#pragma once

#include <cstddef>
#include <cstdint>

namespace consolidator::max
{

struct DspSnapshot
{
    float gain;
};

struct SharedDspExchange
{
    DspSnapshot snapshots[3];
    std::uint32_t publishedIndex{};
    std::uint32_t consumerIndex{};
};

static_assert(sizeof(DspSnapshot) == 4);
static_assert(sizeof(SharedDspExchange) == 20);
static_assert(offsetof(SharedDspExchange, snapshots) == 0);
static_assert(offsetof(SharedDspExchange, publishedIndex) == 12);
static_assert(offsetof(SharedDspExchange, consumerIndex) == 16);

} // namespace consolidator::max
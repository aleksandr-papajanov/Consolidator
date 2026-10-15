#pragma once

#include <cstdint>

#include "SharedDspState.h"

namespace consolidator::transport
{

[[nodiscard]] bool ConsumePublishedDspState(
    DspStateExchange& exchange,
    std::uint32_t& consumerIndex,
    DspSnapshot& destination) noexcept;

} // namespace consolidator::transport

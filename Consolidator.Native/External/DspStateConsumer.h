#pragma once

#include <cstdint>

#include "SharedDspState.h"

namespace consolidator::max
{

[[nodiscard]] bool ConsumePublishedDspState(
    DspStateExchange& exchange,
    std::uint32_t& consumerIndex,
    DspSnapshot& destination) noexcept;

} // namespace consolidator::max

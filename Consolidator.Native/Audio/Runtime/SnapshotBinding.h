#pragma once

#include "../Chain/ConsolidatorChain.h"
#include "../../Transport/SharedDspState.h"

namespace consolidator::audio::runtime
{

// This boundary alone translates transport snapshot fields and filter slot offsets.
[[nodiscard]] ChainParameters BindSnapshot(const transport::DspSnapshot& snapshot) noexcept;

} // namespace consolidator::audio::runtime

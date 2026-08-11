#pragma once

#include <cstdint>
#include <optional>

#include "c74_min_api.h"

namespace consolidator::max
{

// Encodes every wire-level numeric identifier as a decimal Max symbol.
[[nodiscard]] c74::min::atom EncodeWireId(std::uint64_t value);

// Decodes only canonical unsigned decimal Max symbols in the wire range.
[[nodiscard]] std::optional<std::uint64_t> DecodeWireId(
    const c74::min::atom& value);

} // namespace consolidator::max

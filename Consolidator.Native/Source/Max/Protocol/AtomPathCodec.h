#pragma once

#include <cstddef>
#include <optional>

#include "Core/Domain/State/StatePath.h"
#include "c74_min_api.h"

namespace consolidator::max
{

// Converts semantic Max paths to and from Core StatePath values.
class AtomPathCodec
{
public:
    [[nodiscard]] std::optional<core::StatePath> Decode(
        const c74::min::atoms& atoms,
        std::size_t& position,
        std::size_t end,
        core::InstanceId instance) const;

    void Encode(c74::min::atoms& atoms, const core::StatePath& path) const;
};

} // namespace consolidator::max

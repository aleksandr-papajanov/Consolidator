#pragma once

#include <optional>

#include "Core/Domain/State/StateEntry.h"
#include "c74_min_api.h"

namespace consolidator::max
{

// Converts path-directed Max values to and from typed Core state values.
class AtomValueCodec
{
public:
    [[nodiscard]] std::optional<core::StateValue> Decode(
        const c74::min::atom& value,
        const core::StatePath& path) const;

    void Encode(c74::min::atoms& atoms, const core::StateValue& value) const;
};

} // namespace consolidator::max

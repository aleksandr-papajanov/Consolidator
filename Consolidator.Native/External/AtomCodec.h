#pragma once

#include <optional>
#include <vector>

#include "ManagedInterop.h"
#include "c74_min_api.h"

namespace consolidator::max
{

class AtomCodec
{
public:
    [[nodiscard]] static std::optional<std::vector<NativeAtom>> Encode(const c74::min::atoms& atoms);
};

} // namespace consolidator::max
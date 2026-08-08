#pragma once

#include <cstdint>
#include <variant>

namespace consolidator::dsp
{

using ParameterValue =
    std::variant<
        bool,
        std::int32_t,
        float>;

} // namespace consolidator::dsp
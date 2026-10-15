#pragma once

#include <cstdint>

namespace consolidator::audio
{

enum class FilterType : std::uint32_t
{
    None,
    Gain,
    Tilt,
    LowShelf,
    HighShelf,
    Bell
};

// Physical filter controls. This is an audio contract, independent of the Managed ABI layout.
struct FilterParameters
{
    std::uint32_t active{};
    std::uint32_t type{};
    float frequencyHz{};
    float gainDb{};
    float q{};
    float fixedQ{};
};

} // namespace consolidator::audio

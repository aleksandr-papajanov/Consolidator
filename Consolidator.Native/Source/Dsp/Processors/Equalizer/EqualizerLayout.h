#pragma once

#include <array>
#include <cstddef>

namespace consolidator::dsp
{

enum class EqualizerFilterKind
{
    Bell,
    LowShelf,
    HighShelf,
    Tilt,
    Gain
};

inline constexpr std::size_t kStandardEqualizerBandCount = 7;

inline constexpr std::array<EqualizerFilterKind, kStandardEqualizerBandCount>
    kStandardEqualizerLayout{
        EqualizerFilterKind::Gain,
        EqualizerFilterKind::Tilt,
        EqualizerFilterKind::LowShelf,
        EqualizerFilterKind::HighShelf,
        EqualizerFilterKind::Bell,
        EqualizerFilterKind::Bell,
        EqualizerFilterKind::Bell};

} // namespace consolidator::dsp

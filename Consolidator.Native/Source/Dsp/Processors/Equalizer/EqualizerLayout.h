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

enum class DetectorFilterKind
{
    LowShelf,
    Bell
};

inline constexpr std::array<DetectorFilterKind, 2>
    kCompressorDetectorLayout{
        DetectorFilterKind::LowShelf,
        DetectorFilterKind::Bell};

inline constexpr std::array<DetectorFilterKind, 2>
    kSaturatorDetectorLayout{
        DetectorFilterKind::LowShelf,
        DetectorFilterKind::Bell};

} // namespace consolidator::dsp

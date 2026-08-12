#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace consolidator::analysis
{

inline constexpr std::size_t kFftSize = 1024;
inline constexpr std::size_t kSpectrumBinCount = kFftSize / 2 + 1;
inline constexpr std::size_t kDisplaySpectrumBinCount = 256;

struct AudioWindow
{
    std::array<float, kFftSize> samples{};
    double sampleRate = 0.0;
    std::uint64_t revision = 0;
};

struct RawSpectrum
{
    std::array<float, kSpectrumBinCount> magnitudes{};
    double sampleRate = 0.0;
    std::uint64_t revision = 0;
};

struct SpectrumSnapshot
{
    std::array<float, kDisplaySpectrumBinCount> magnitudeDb{};
    double sampleRate = 0.0;
    std::uint64_t revision = 0;
};

} // namespace consolidator::analysis

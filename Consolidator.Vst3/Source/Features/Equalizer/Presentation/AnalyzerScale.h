#pragma once

#include <cmath>

namespace consolidator::vst3::features::equalizer::presentation
{

// Normalized plot coordinates shared by response, spectrum and pointer mapping.
struct AnalyzerScale
{
    [[nodiscard]] static double Frequency(double position)
    {
        return 20 * std::pow(1000.0, position);
    }

    [[nodiscard]] static double FrequencyPosition(double frequency)
    {
        return std::log(frequency / 20) / std::log(1000.0);
    }

    [[nodiscard]] static constexpr double Gain(double position)
    {
        return 24 - 48 * position;
    }

    [[nodiscard]] static constexpr double GainPosition(double gain)
    {
        return 0.5 - gain / 48;
    }
};

} // namespace consolidator::vst3::features::equalizer::presentation

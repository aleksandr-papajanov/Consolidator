#pragma once

#include "../Settings/AnalysisOptions.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace consolidator::helpers {

class NumericHelper final {
public:
    template <typename Value>
    static Value Clamp(Value value, Value minimum, Value maximum) {
        return std::clamp(value, minimum, maximum);
    }

    static double PositiveOr(double value, double fallback) {
        return std::isfinite(value) && value > 0.0 ? value : fallback;
    }

    static double ClampFinite(double value, double minimum, double maximum, double fallback) {
        return std::isfinite(value) ? Clamp(value, minimum, maximum) : fallback;
    }

    static double AtLeast(double value, double minimum) {
        return std::isfinite(value) ? std::max(value, minimum) : minimum;
    }

    static double SafeDenominator(double value, double fallback = 1.0) {
        return std::isfinite(value) && std::abs(value) >= MinimumMagnitude ? value : fallback;
    }

    static bool IsStrictlyIncreasingRange(double minimum, double maximum) {
        return std::isfinite(minimum) && std::isfinite(maximum) && minimum < maximum;
    }

    static bool IsPowerOfTwo(std::size_t value) {
        return value >= 2 && (value & (value - 1)) == 0;
    }

    static std::size_t FloorPowerOfTwo(std::size_t value) {
        if (value < 2) return 0;
        std::size_t result = 1;
        while (result <= value / 2) result *= 2;
        return result;
    }

    static double UnitToLinear(double value, double minimum, double maximum) {
        return minimum + Clamp(value, 0.0, 1.0) * (maximum - minimum);
    }

    static double LinearToUnit(double value, double minimum, double maximum) {
        if (maximum <= minimum) return 0.0;
        return Clamp((value - minimum) / (maximum - minimum), 0.0, 1.0);
    }

    static double UnitToLogarithmic(double value, double minimum, double maximum) {
        if (minimum <= 0.0 || maximum <= minimum) return minimum;
        return minimum * std::pow(maximum / minimum, Clamp(value, 0.0, 1.0));
    }

    static double LogarithmicToUnit(double value, double minimum, double maximum) {
        if (value <= 0.0 || minimum <= 0.0 || maximum <= minimum) return 0.0;
        return Clamp(std::log(value / minimum) / std::log(maximum / minimum), 0.0, 1.0);
    }

    static double DecibelsToMagnitude(double decibels) {
        return std::pow(10.0, decibels / 20.0);
    }

    static double MagnitudeToDecibels(
        double magnitude,
        double noiseFloor = settings::AnalysisOptions::MagnitudeNoiseFloor
    ) {
        return 20.0 * std::log10(std::max(0.0, magnitude) + std::max(0.0, noiseFloor));
    }

    static double GainDbToBiquadAmplitude(double gainDb) {
        return std::pow(10.0, gainDb / 40.0);
    }

private:
    static constexpr double MinimumMagnitude = settings::AnalysisOptions::MinimumMagnitude;
};

} // namespace consolidator::helpers

#pragma once

#include <span>

struct SignalAnalysisWindow final {
    std::span<const double> left;
    std::span<const double> right;
    std::span<const double> spectrumMagnitudes;
    std::span<const double> leftPowers;
    std::span<const double> rightPowers;
    std::span<const double> crossPowers;
    double sampleRate = 0.0;

    double FrequencyForBin(std::size_t index) const {
        return spectrumMagnitudes.empty()
            ? 0.0
            : static_cast<double>(index) * sampleRate /
                static_cast<double>(spectrumMagnitudes.size() * 2);
    }
};

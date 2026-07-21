#pragma once

#include "Settings/AnalysisOptions.h"

#include <array>
#include <cstddef>

struct AnalyzerSignalSpectrum final {
    static constexpr std::size_t MaximumBinCount =
        consolidator::settings::AnalysisOptions::MaximumFftSize / 2;

    std::array<double, MaximumBinCount> magnitudes{};
    std::array<double, MaximumBinCount> decibels{};
    std::array<double, MaximumBinCount> leftPowers{};
    std::array<double, MaximumBinCount> rightPowers{};
    std::array<double, MaximumBinCount> crossPowers{};
    std::size_t pointCount = 0;
};

struct AnalyzerSpectrumResult final {
    AnalyzerSignalSpectrum current;
    AnalyzerSignalSpectrum reference;
};

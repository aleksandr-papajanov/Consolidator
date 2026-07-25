#pragma once

#include <cstddef>
namespace consolidator::settings {

struct AnalysisOptions final {
    static constexpr std::size_t DefaultFftSize = 4096;
    static constexpr std::size_t MaximumFftSize = 8192;
    static constexpr std::size_t DefaultCurvePointCount = 128;
    static constexpr double DefaultSpectrumSmoothing = 0.5;
    static constexpr double DefaultLowFrequencySmoothing = 1.0;
    static constexpr double MaximumCurveSmoothing = 0.9997;
    static constexpr double LowFrequencySmoothingExponent = 2.5;
    static constexpr double HighFrequencyTiltExponent = 1;
    static constexpr double SpectralSimilarityScaleDb = 12.0;
    static constexpr double HannWindowCoefficient = 0.5;
    static constexpr double HannWindowCoherentGain = 0.5;
    static constexpr double SingleSidedSpectrumScale = 0.5;
    static constexpr double MagnitudeNoiseFloor = 1.0e-12;
    static constexpr double MinimumMagnitude = 1.0e-20;
};

} // namespace consolidator::settings

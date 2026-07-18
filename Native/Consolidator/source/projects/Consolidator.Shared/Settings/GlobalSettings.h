#pragma once

#include <cstddef>

namespace consolidator::settings {

class GlobalSettings final {
public:
    static constexpr double DefaultSampleRateHz = 48000.0;
    static constexpr double DefaultFrequencyHz = 1000.0;
    static constexpr double DefaultFilterQ = 0.707;
    static constexpr double MinimumFrequencyHz = 20.0;
    static constexpr double MaximumFrequencyHz = 20000.0;
    static constexpr std::size_t DefaultCurvePointCount = 128;
    static constexpr std::size_t DefaultFftSize = 4096;
    static constexpr std::size_t MaximumFftSize = 8192;
    static constexpr double DefaultSpectrumSmoothing = 0.4;
    static constexpr double DefaultLowFrequencySmoothing = 1.0;
    static constexpr double DefaultSpectrumCalibrationDb = 30.0;
    static constexpr double DefaultSpectrumTiltDb = 24.0;
    static constexpr double MinimumSpectrumDb = -120.0;
    static constexpr double MaximumSpectrumDb = 48.0;
    static constexpr double MinimumDifferenceDb = -60.0;
    static constexpr double MaximumDifferenceDb = 60.0;
    static constexpr double MaximumCurveSmoothing = 0.9997;
    static constexpr double LowFrequencySmoothingExponent = 2.5;
    static constexpr double HighFrequencyTiltExponent = 1.35;
    static constexpr double HannWindowCoefficient = 0.5;
    static constexpr double HannWindowCoherentGain = 0.5;
    static constexpr double SingleSidedSpectrumScale = 0.5;
    static constexpr double MagnitudeNoiseFloor = 1e-12;
    static constexpr double MinimumMagnitude = 1e-20;
    static constexpr double MinimumBiquadFrequencyHz = 1e-6;
    static constexpr double MinimumBiquadQ = 1e-6;
    static constexpr double MaximumBiquadFrequencyRatio = 0.499999;
    static constexpr std::size_t MaximumFilterCount = 16;
};

} // namespace consolidator::settings

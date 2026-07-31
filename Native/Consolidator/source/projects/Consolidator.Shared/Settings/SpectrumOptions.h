#pragma once

namespace consolidator::settings {

struct SpectrumOptions final {
    static constexpr double MinimumFrequencyHz = 20.0;
    static constexpr double MaximumFrequencyHz = 20000.0;
    static constexpr double DefaultSpectrumCalibrationDb = 30.0;
    static constexpr double MinimumSpectrumDb = -120.0;
    static constexpr double MaximumSpectrumDb = 48.0;
    static constexpr double MinimumDifferenceDb = -60.0;
    static constexpr double MaximumDifferenceDb = 60.0;
};

} // namespace consolidator::settings

#pragma once

#include <cstddef>
namespace consolidator::settings {

struct EqOptions final {
    static constexpr double DefaultFrequencyHz = 1000.0;
    static constexpr double DefaultFilterQ = 0.707;
    static constexpr double MinimumBiquadFrequencyHz = 1.0e-6;
    static constexpr double MinimumBiquadQ = 1.0e-6;
    static constexpr double MaximumBiquadFrequencyRatio = 0.499999;
    static constexpr std::size_t MaximumFilterCount = 16;
};

} // namespace consolidator::settings

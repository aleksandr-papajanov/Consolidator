#pragma once

#include <algorithm>
#include <cstddef>

namespace consolidator::settings {

struct AudioOptions final {
    static constexpr double DefaultSampleRateHz = 48000.0;
    static constexpr double ParameterSmoothingMilliseconds = 5.0;
    static constexpr std::size_t ProcessorTelemetryWindowSamples = 2048;

    static std::size_t ParameterSmoothingSamples(double sampleRate) {
        return std::max<std::size_t>(1, static_cast<std::size_t>(
            sampleRate * ParameterSmoothingMilliseconds / 1000.0));
    }
};

} // namespace consolidator::settings

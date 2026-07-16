#pragma once

#include <cmath>
#include <vector>

namespace EqCurveGrid {
constexpr std::size_t point_count = 128;
constexpr double min_hz = 20.0;
constexpr double max_hz = 20000.0;
constexpr double default_sample_rate = 48000.0;
}

inline std::vector<double> make_log_frequency_grid(size_t count, double min_hz, double max_hz) {
    std::vector<double> result;
    result.reserve(count);

    const double min_log = std::log(min_hz);
    const double max_log = std::log(max_hz);

    for (size_t i = 0; i < count; ++i) {
        const double t = count <= 1
            ? 0.0
            : static_cast<double>(i) / static_cast<double>(count - 1);

        result.push_back(std::exp(min_log + t * (max_log - min_log)));
    }

    return result;
}

inline std::vector<double> make_eq_curve_frequency_grid(std::size_t count = EqCurveGrid::point_count) {
    return make_log_frequency_grid(count, EqCurveGrid::min_hz, EqCurveGrid::max_hz);
}

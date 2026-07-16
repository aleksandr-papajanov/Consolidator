#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

enum class ParameterScale {
    linear,
    logarithmic,
    discrete
};

struct ParameterRange {
    double min_value = 0.0;
    double max_value = 1.0;
    ParameterScale scale = ParameterScale::linear;
    std::vector<double> discrete_values;
};

inline double normalize_parameter(const ParameterRange& range, double value) {
    if (range.max_value <= range.min_value) {
        return 0.0;
    }

    const double clamped_value = std::clamp(value, range.min_value, range.max_value);

    if (range.scale == ParameterScale::discrete) {
        if (range.discrete_values.empty()) {
            return 0.0;
        }

        const auto nearest = std::min_element(
            range.discrete_values.begin(),
            range.discrete_values.end(),
            [clamped_value](double left, double right) {
                return std::abs(left - clamped_value) < std::abs(right - clamped_value);
            });

        const auto index = static_cast<double>(std::distance(range.discrete_values.begin(), nearest));
        return range.discrete_values.size() <= 1
            ? 0.0
            : index / static_cast<double>(range.discrete_values.size() - 1);
    }

    double unit = 0.0;

    switch (range.scale) {
        case ParameterScale::linear:
            unit = (clamped_value - range.min_value) / (range.max_value - range.min_value);
            break;
        case ParameterScale::logarithmic: {
            if (clamped_value <= 0.0 || range.min_value <= 0.0 || range.max_value <= 0.0) {
                return 0.0;
            }

            const double log_min = std::log(range.min_value);
            const double log_max = std::log(range.max_value);
            const double log_value = std::log(clamped_value);
            unit = (log_value - log_min) / (log_max - log_min);
            break;
        }
        case ParameterScale::discrete:
            break;
    }

    return unit;
}

inline double denormalize_parameter(const ParameterRange& range, double normalized_value) {
    const double t = std::clamp(normalized_value, 0.0, 1.0);

    if (range.scale == ParameterScale::discrete) {
        if (range.discrete_values.empty()) {
            return range.min_value;
        }

        const auto index = static_cast<std::size_t>(std::round(
            t * static_cast<double>(range.discrete_values.size() - 1)));
        return range.discrete_values[std::min(index, range.discrete_values.size() - 1)];
    }

    switch (range.scale) {
        case ParameterScale::linear:
            return range.min_value + t * (range.max_value - range.min_value);
        case ParameterScale::logarithmic:
            if (range.min_value <= 0.0 || range.max_value <= 0.0 || range.max_value <= range.min_value) {
                return range.min_value;
            }

            return range.min_value * std::pow(range.max_value / range.min_value, t);
        case ParameterScale::discrete:
            return range.min_value;
    }

    return range.min_value;
}

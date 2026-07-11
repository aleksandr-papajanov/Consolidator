#pragma once

#include "TargetCurve.h"

#include <algorithm>
#include <cmath>
#include <vector>

inline double interpolate_log_curve_value(
    const std::vector<double>& source_frequencies,
    const std::vector<double>& source_values,
    double frequency_hz
) {
    if (source_frequencies.empty() || source_values.empty()) {
        return 0.0;
    }

    if (source_frequencies.size() != source_values.size()) {
        return 0.0;
    }

    if (frequency_hz <= source_frequencies.front()) {
        return source_values.front();
    }

    if (frequency_hz >= source_frequencies.back()) {
        return source_values.back();
    }

    auto upper = std::lower_bound(
        source_frequencies.begin(),
        source_frequencies.end(),
        frequency_hz
    );

    const size_t upper_index = static_cast<size_t>(std::distance(source_frequencies.begin(), upper));
    if (upper_index == 0 || upper_index >= source_frequencies.size()) {
        return source_values.back();
    }

    const size_t lower_index = upper_index - 1;
    const double lower_freq = source_frequencies[lower_index];
    const double upper_freq = source_frequencies[upper_index];
    const double lower_value = source_values[lower_index];
    const double upper_value = source_values[upper_index];

    if (upper_freq <= lower_freq) {
        return lower_value;
    }

    const double log_lower = std::log(lower_freq);
    const double log_upper = std::log(upper_freq);
    const double log_target = std::log(frequency_hz);
    const double t = (log_target - log_lower) / (log_upper - log_lower);

    return lower_value + t * (upper_value - lower_value);
}

inline std::vector<double> resample_log_curve(
    const std::vector<double>& source_frequencies,
    const std::vector<double>& source_values,
    const std::vector<double>& target_frequencies
) {
    std::vector<double> result;
    result.reserve(target_frequencies.size());

    for (double frequency_hz : target_frequencies) {
        result.push_back(interpolate_log_curve_value(source_frequencies, source_values, frequency_hz));
    }

    return result;
}

inline TargetCurve subtract_curves(
    const TargetCurve& target,
    const TargetCurve& baseline
) {
    TargetCurve result;
    result.frequencies = target.frequencies;
    result.values.reserve(target.values.size());

    const auto baseline_at_target = resample_log_curve(
        baseline.frequencies,
        baseline.values,
        target.frequencies
    );

    for (size_t i = 0; i < target.values.size(); ++i) {
        result.values.push_back(target.values[i] - baseline_at_target[i]);
    }

    return result;
}

inline TargetCurve add_curves(
    const TargetCurve& target,
    const TargetCurve& baseline
) {
    TargetCurve result;
    result.frequencies = target.frequencies;
    result.values.reserve(target.values.size());

    const auto baseline_at_target = resample_log_curve(
        baseline.frequencies,
        baseline.values,
        target.frequencies
    );

    for (size_t i = 0; i < target.values.size(); ++i) {
        result.values.push_back(target.values[i] + baseline_at_target[i]);
    }

    return result;
}

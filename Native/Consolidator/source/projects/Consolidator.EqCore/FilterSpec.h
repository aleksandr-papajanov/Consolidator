#pragma once

#include "EqBiquad.h"
#include "EqFrequencyGrid.h"

#include <string>

enum class FilterType {
    gain,
    tilt,
    peak,
    low_shelf,
    high_shelf
};

struct FilterSpec {
    FilterType type = FilterType::peak;
    double gainDb = 0.0;
    double freqHz = 1000.0;
    double q = 1.0;
    double pivotHz = 1000.0;
};

inline const char* filter_type_name(FilterType type) {
    switch (type) {
        case FilterType::gain: return "gain";
        case FilterType::tilt: return "tilt";
        case FilterType::peak: return "peak";
        case FilterType::low_shelf: return "lowshelf";
        case FilterType::high_shelf: return "highshelf";
    }

    return "peak";
}

inline FilterType filter_type_from_symbol(const std::string& value) {
    if (value == "gain") {
        return FilterType::gain;
    }

    if (value == "tilt") {
        return FilterType::tilt;
    }

    if (value == "lowshelf" || value == "low_shelf") {
        return FilterType::low_shelf;
    }

    if (value == "highshelf" || value == "high_shelf") {
        return FilterType::high_shelf;
    }

    return FilterType::peak;
}

inline EqBiquadCoefficients filter_coefficients(
    const FilterSpec& spec,
    double sample_rate
) {
    switch (spec.type) {
        case FilterType::gain:
        case FilterType::tilt:
            break;
        case FilterType::low_shelf:
            return EqBiquad::low_shelf(spec.freqHz, spec.q, spec.gainDb, sample_rate);
        case FilterType::high_shelf:
            return EqBiquad::high_shelf(spec.freqHz, spec.q, spec.gainDb, sample_rate);
        case FilterType::peak:
        default:
            return EqBiquad::peak(spec.freqHz, spec.q, spec.gainDb, sample_rate);
    }

    return {};
}

inline double filter_response_db(
    const FilterSpec& spec,
    double frequency_hz,
    double sample_rate
) {
    switch (spec.type) {
        case FilterType::gain:
            return spec.gainDb;
        case FilterType::tilt:
            return EqBiquad::response_db(
                frequency_hz,
                sample_rate,
                EqBiquad::low_shelf(spec.pivotHz, 0.707, -spec.gainDb, sample_rate)
            ) + EqBiquad::response_db(
                frequency_hz,
                sample_rate,
                EqBiquad::high_shelf(spec.pivotHz, 0.707, spec.gainDb, sample_rate)
            );
        case FilterType::low_shelf:
            return EqBiquad::response_db(frequency_hz, sample_rate, filter_coefficients(spec, sample_rate));
        case FilterType::high_shelf:
            return EqBiquad::response_db(frequency_hz, sample_rate, filter_coefficients(spec, sample_rate));
        case FilterType::peak:
        default:
            return EqBiquad::response_db(frequency_hz, sample_rate, filter_coefficients(spec, sample_rate));
    }
}

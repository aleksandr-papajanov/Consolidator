#pragma once

#include "EqParams.h"

#include <algorithm>
#include <cmath>

namespace EqRanges {
    inline constexpr double gain_min_db = -18.0;
    inline constexpr double gain_max_db = 18.0;

    inline constexpr double tilt_min_db = -18.0;
    inline constexpr double tilt_max_db = 18.0;
    inline constexpr double tilt_pivot_min_hz = 200.0;
    inline constexpr double tilt_pivot_max_hz = 4000.0;

    inline constexpr double shelf_gain_min_db = -18.0;
    inline constexpr double shelf_gain_max_db = 18.0;
    inline constexpr double shelf_freq_min_hz = 30.0;
    inline constexpr double shelf_freq_max_hz = 18000.0;
    inline constexpr double shelf_q_min = 0.2;
    inline constexpr double shelf_q_max = 2.0;

    inline constexpr double bell_gain_min_db = -18.0;
    inline constexpr double bell_gain_max_db = 18.0;
    inline constexpr double bell_freq_min_hz = 40.0;
    inline constexpr double bell_freq_max_hz = 18000.0;
    inline constexpr double bell_q_min = 0.2;
    inline constexpr double bell_q_max = 8.0;

    inline double clamp_unit(double value) {
        return std::clamp(value, 0.0, 1.0);
    }

    inline double unit_to_linear(double value, double min_value, double max_value) {
        return min_value + clamp_unit(value) * (max_value - min_value);
    }

    inline double linear_to_unit(double value, double min_value, double max_value) {
        if (max_value <= min_value) {
            return 0.0;
        }

        return clamp_unit((value - min_value) / (max_value - min_value));
    }

    inline double unit_to_log(double value, double min_value, double max_value) {
        if (min_value <= 0.0 || max_value <= 0.0 || max_value <= min_value) {
            return min_value;
        }

        const double t = clamp_unit(value);
        return min_value * std::pow(max_value / min_value, t);
    }

    inline double log_to_unit(double value, double min_value, double max_value) {
        if (min_value <= 0.0 || max_value <= 0.0 || max_value <= min_value || value <= 0.0) {
            return 0.0;
        }

        const double log_min = std::log(min_value);
        const double log_max = std::log(max_value);
        const double log_value = std::log(value);
        return clamp_unit((log_value - log_min) / (log_max - log_min));
    }

    inline void clamp(EqParams& params) {
        params.gainDb = std::clamp(params.gainDb, gain_min_db, gain_max_db);

        params.tiltDb = std::clamp(params.tiltDb, tilt_min_db, tilt_max_db);
        params.tiltPivotHz = std::clamp(params.tiltPivotHz, tilt_pivot_min_hz, tilt_pivot_max_hz);

        params.lowShelf.gainDb = std::clamp(params.lowShelf.gainDb, shelf_gain_min_db, shelf_gain_max_db);
        params.lowShelf.freqHz = std::clamp(params.lowShelf.freqHz, shelf_freq_min_hz, shelf_freq_max_hz);
        params.lowShelf.q = std::clamp(params.lowShelf.q, shelf_q_min, shelf_q_max);

        params.highShelf.gainDb = std::clamp(params.highShelf.gainDb, shelf_gain_min_db, shelf_gain_max_db);
        params.highShelf.freqHz = std::clamp(params.highShelf.freqHz, shelf_freq_min_hz, shelf_freq_max_hz);
        params.highShelf.q = std::clamp(params.highShelf.q, shelf_q_min, shelf_q_max);

        for (auto& bell : params.bells) {
            bell.gainDb = std::clamp(bell.gainDb, bell_gain_min_db, bell_gain_max_db);
            bell.freqHz = std::clamp(bell.freqHz, bell_freq_min_hz, bell_freq_max_hz);
            bell.q = std::clamp(bell.q, bell_q_min, bell_q_max);
        }
    }

    inline EqParams defaults() {
        EqParams params;

        params.gainDb = 0.0;

        params.tiltDb = 0.0;
        params.tiltPivotHz = 1000.0;

        params.lowShelf.gainDb = 0.0;
        params.lowShelf.freqHz = 120.0;
        params.lowShelf.q = 0.707;

        params.highShelf.gainDb = 0.0;
        params.highShelf.freqHz = 8000.0;
        params.highShelf.q = 0.707;

        const double bell_freqs[4] = { 250.0, 800.0, 2500.0, 7000.0 };
        for (int i = 0; i < 4; ++i) {
            params.bells[i].gainDb = 0.0;
            params.bells[i].freqHz = bell_freqs[i];
            params.bells[i].q = 1.0;
        }

        return params;
    }
}

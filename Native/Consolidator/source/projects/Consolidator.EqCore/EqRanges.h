#pragma once

#include "EqParams.h"

#include <algorithm>

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

#pragma once

#include "EqRanges.h"
#include "EqFilterBank.h"
#include "EqFrequencyGrid.h"
#include "EqParams.h"

#include <vector>

class EqState {
public:
    EqState() :
        frequencies_{ make_log_frequency_grid(128, 20.0, 20000.0) } {
        preview_bank_.set_sample_rate(sample_rate_);
    }

    void set_sample_rate(double sample_rate) {
        sample_rate_ = sample_rate;
        preview_bank_.set_sample_rate(sample_rate_);
    }

    void set_gain(double gain_db) {
        params_.gainDb = gain_db;
        EqRanges::clamp(params_);
        preview_bank_.set_params(params_);
    }

    void set_tilt(double tilt_db, double pivot_hz) {
        params_.tiltDb = tilt_db;
        params_.tiltPivotHz = pivot_hz;
        EqRanges::clamp(params_);
        preview_bank_.set_params(params_);
    }

    void set_low_shelf(double gain_db, double freq_hz, double q) {
        params_.lowShelf.gainDb = gain_db;
        params_.lowShelf.freqHz = freq_hz;
        params_.lowShelf.q = q;
        EqRanges::clamp(params_);
        preview_bank_.set_params(params_);
    }

    void set_high_shelf(double gain_db, double freq_hz, double q) {
        params_.highShelf.gainDb = gain_db;
        params_.highShelf.freqHz = freq_hz;
        params_.highShelf.q = q;
        EqRanges::clamp(params_);
        preview_bank_.set_params(params_);
    }

    void set_bell(int index, double gain_db, double freq_hz, double q) {
        if (index < 0 || index >= static_cast<int>(params_.bells.size())) {
            return;
        }

        params_.bells[static_cast<size_t>(index)].gainDb = gain_db;
        params_.bells[static_cast<size_t>(index)].freqHz = freq_hz;
        params_.bells[static_cast<size_t>(index)].q = q;
        EqRanges::clamp(params_);
        preview_bank_.set_params(params_);
    }

    void reset() {
        params_ = EqRanges::defaults();
        preview_bank_.set_params(params_);
    }

    const EqParams& params() const {
        return params_;
    }

    std::vector<double> curve() const {
        return preview_bank_.response_curve(frequencies_);
    }

private:
    double sample_rate_ = 48000.0;
    EqParams params_{};
    std::vector<double> frequencies_;
    EqFilterBank preview_bank_;
};

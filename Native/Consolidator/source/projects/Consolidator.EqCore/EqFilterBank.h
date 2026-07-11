#pragma once

#include "EqBiquad.h"
#include "EqParams.h"

#include <array>
#include <vector>

class EqFilterBank {
public:
    static constexpr int filter_count = 8;

    void set_sample_rate(double sample_rate) {
        if (sample_rate_ != sample_rate) {
            sample_rate_ = sample_rate;
            dirty_ = true;
        }
    }

    void set_params(const EqParams& params) {
        params_ = params;
        dirty_ = true;
    }

    const std::array<EqBiquadCoefficients, filter_count>& coefficients() const {
        rebuild_if_needed();
        return coefficients_;
    }

    double input_gain() const {
        rebuild_if_needed();
        return input_gain_;
    }

    std::vector<double> response_curve(const std::vector<double>& freqs) const {
        rebuild_if_needed();

        std::vector<double> result;
        result.reserve(freqs.size());

        for (double f : freqs) {
            double db = 20.0 * std::log10(std::max(1e-12, input_gain_));

            for (const auto& coeff : coefficients_) {
                db += EqBiquad::response_db(f, sample_rate_, coeff);
            }

            result.push_back(db);
        }

        return result;
    }

private:
    void rebuild_if_needed() const {
        if (!dirty_) {
            return;
        }

        input_gain_ = std::pow(10.0, params_.gainDb / 20.0);

        coefficients_[0] = EqBiquad::low_shelf(params_.tiltPivotHz, 0.707, -params_.tiltDb * 0.5, sample_rate_);
        coefficients_[1] = EqBiquad::high_shelf(params_.tiltPivotHz, 0.707, params_.tiltDb * 0.5, sample_rate_);
        coefficients_[2] = EqBiquad::low_shelf(params_.lowShelf.freqHz, params_.lowShelf.q, params_.lowShelf.gainDb, sample_rate_);
        coefficients_[3] = EqBiquad::peak(params_.bells[0].freqHz, params_.bells[0].q, params_.bells[0].gainDb, sample_rate_);
        coefficients_[4] = EqBiquad::peak(params_.bells[1].freqHz, params_.bells[1].q, params_.bells[1].gainDb, sample_rate_);
        coefficients_[5] = EqBiquad::peak(params_.bells[2].freqHz, params_.bells[2].q, params_.bells[2].gainDb, sample_rate_);
        coefficients_[6] = EqBiquad::peak(params_.bells[3].freqHz, params_.bells[3].q, params_.bells[3].gainDb, sample_rate_);
        coefficients_[7] = EqBiquad::high_shelf(params_.highShelf.freqHz, params_.highShelf.q, params_.highShelf.gainDb, sample_rate_);

        dirty_ = false;
    }

    mutable double sample_rate_ = 48000.0;
    mutable bool dirty_ = true;
    mutable EqParams params_{};
    mutable std::array<EqBiquadCoefficients, filter_count> coefficients_{};
    mutable double input_gain_ = 1.0;
};

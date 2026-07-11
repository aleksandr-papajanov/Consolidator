#pragma once

#include "c74_min.h"

#include "CurveInterpolation.h"
#include "TargetCurve.h"
#include "EqFrequencyGrid.h"

class ApproximatorCurveStore {
public:
    void set_target(const c74::min::atoms& args) {
        live_curve_.values.clear();
        live_curve_.values.reserve(args.size());

        for (const auto& a : args) {
            live_curve_.values.push_back(static_cast<double>(a));
        }

        live_curve_.frequencies = make_log_frequency_grid(live_curve_.values.size(), 20.0, 20000.0);
        has_live_curve_ = true;
    }

    void set_baseline(const c74::min::atoms& args) {
        live_baseline_.values.clear();
        live_baseline_.values.reserve(args.size());

        for (const auto& a : args) {
            live_baseline_.values.push_back(static_cast<double>(a));
        }

        live_baseline_.frequencies = make_log_frequency_grid(live_baseline_.values.size(), 20.0, 20000.0);
        has_live_baseline_ = true;
    }

    bool capture() {
        if (!has_live_curve_) {
            return false;
        }

        if (!has_live_baseline_) {
            return false;
        }

        captured_curve_ = live_curve_;
        captured_baseline_ = live_baseline_;
        has_captured_curve_ = true;
        return true;
    }

    void clear() {
        live_curve_.frequencies.clear();
        live_curve_.values.clear();
        live_baseline_.frequencies.clear();
        live_baseline_.values.clear();
        captured_curve_.frequencies.clear();
        captured_curve_.values.clear();
        captured_baseline_.frequencies.clear();
        captured_baseline_.values.clear();
        has_live_curve_ = false;
        has_live_baseline_ = false;
        has_captured_curve_ = false;
    }

    bool has_live_curve() const {
        return has_live_curve_;
    }

    bool has_live_baseline() const {
        return has_live_baseline_;
    }

    bool has_captured_curve() const {
        return has_captured_curve_;
    }

    const TargetCurve& live_curve() const {
        return live_curve_;
    }

    const TargetCurve& captured_curve() const {
        return captured_curve_;
    }

    const TargetCurve& captured_baseline_curve() const {
        return captured_baseline_;
    }

    TargetCurve captured_residual_curve() const {
        return add_curves(captured_curve_, captured_baseline_);
    }

    const std::vector<double>& freqs() const {
        return live_curve_.frequencies;
    }

    const std::vector<double>& target_db() const {
        return live_curve_.values;
    }

private:
    TargetCurve live_curve_;
    TargetCurve live_baseline_;
    TargetCurve captured_curve_;
    TargetCurve captured_baseline_;
    bool has_live_curve_ = false;
    bool has_live_baseline_ = false;
    bool has_captured_curve_ = false;
};

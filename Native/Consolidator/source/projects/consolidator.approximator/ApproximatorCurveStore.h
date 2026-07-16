#pragma once

#include "c74_min.h"

#include "TargetCurve.h"
#include "EqFrequencyGrid.h"

#include <stdexcept>

class ApproximatorCurveStore {
public:
    void set_target(const c74::min::atoms& args) {
        assign_curve(difference_curve_, has_difference_curve_, args);
    }

    void set_current_eq(const c74::min::atoms& args) {
        assign_curve(current_eq_curve_, has_current_eq_curve_, args);
    }

    void clear() {
        difference_curve_ = {};
        current_eq_curve_ = {};
        has_difference_curve_ = false;
        has_current_eq_curve_ = false;
    }

    bool has_live_curve() const {
        return has_difference_curve_;
    }

    bool has_current_eq_curve() const {
        return has_current_eq_curve_;
    }

    bool has_compatible_curves() const {
        return has_difference_curve_ &&
            has_current_eq_curve_ &&
            difference_curve_.values.size() == current_eq_curve_.values.size();
    }

    const TargetCurve& live_curve() const {
        return difference_curve_;
    }

    TargetCurve combined_curve() const {
        if (!has_difference_curve_ || !has_current_eq_curve_) {
            throw std::runtime_error("missing_curve_input");
        }

        if (difference_curve_.values.size() != current_eq_curve_.values.size()) {
            throw std::runtime_error("curve_size_mismatch");
        }

        TargetCurve result = difference_curve_;
        for (std::size_t i = 0; i < result.values.size(); ++i) {
            result.values[i] += current_eq_curve_.values[i];
        }

        return result;
    }

    const std::vector<double>& freqs() const {
        return difference_curve_.frequencies;
    }

    const std::vector<double>& target_db() const {
        return difference_curve_.values;
    }

private:
    static void assign_curve(
        TargetCurve& target,
        bool& available,
        const c74::min::atoms& args
    ) {
        target.values.clear();
        target.values.reserve(args.size());

        for (const auto& a : args) {
            target.values.push_back(static_cast<double>(a));
        }

        target.frequencies = make_log_frequency_grid(
            EqCurveGrid::point_count,
            EqCurveGrid::min_hz,
            EqCurveGrid::max_hz);
        available = args.size() == EqCurveGrid::point_count;
        if (!available) {
            target.values.clear();
            target.frequencies.clear();
        }
    }

    TargetCurve difference_curve_;
    TargetCurve current_eq_curve_;
    bool has_difference_curve_ = false;
    bool has_current_eq_curve_ = false;
};

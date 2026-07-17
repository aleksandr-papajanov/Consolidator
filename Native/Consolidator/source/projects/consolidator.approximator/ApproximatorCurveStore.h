#pragma once

#include "c74_min.h"

#include "TargetCurve.h"
#include "EqFrequencyGrid.h"

#include <stdexcept>

class ApproximatorCurveStore {
public:
    void SetTarget(const c74::min::atoms& args) {
        AssignCurve(difference_curve_, has_difference_curve_, args);
    }

    void SetCurrentEq(const c74::min::atoms& args) {
        AssignCurve(current_eq_curve_, has_current_eq_curve_, args);
    }

    void ClearTarget() {
        difference_curve_ = {};
        has_difference_curve_ = false;
    }

    bool HasTarget() const {
        return has_difference_curve_;
    }

    bool HasCurrentEq() const {
        return has_current_eq_curve_;
    }

    bool HasCompatibleCurves() const {
        return has_difference_curve_ &&
            has_current_eq_curve_ &&
            difference_curve_.values.size() == current_eq_curve_.values.size();
    }

    TargetCurve CombinedCurve() const {
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

private:
    static void AssignCurve(
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

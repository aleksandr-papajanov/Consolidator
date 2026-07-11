#pragma once

#include "AnalyzerFrameBuffer.h"

#include "c74_min.h"

#include <algorithm>
#include <cmath>

class AnalyzerStatistics {
public:
    void accumulate(const AnalyzerInputFrame& frame) {
        const double delta_l = frame.reference.left - frame.current.left;
        const double delta_r = frame.reference.right - frame.current.right;

        current_sum_sq_ += average_power(frame.current.left, frame.current.right);
        reference_sum_sq_ += average_power(frame.reference.left, frame.reference.right);
        delta_sum_sq_ += average_power(delta_l, delta_r);
        delta_peak_ = std::max(delta_peak_, std::max(std::abs(delta_l), std::abs(delta_r)));
        ++sample_count_;
    }

    c74::min::atoms build_atoms() const {
        c74::min::atoms stats_atoms;

        if (sample_count_ <= 0) {
            stats_atoms.push_back(0.0);
            stats_atoms.push_back(0.0);
            stats_atoms.push_back(0.0);
            stats_atoms.push_back(0.0);
            return stats_atoms;
        }

        const double sample_count = static_cast<double>(sample_count_);

        stats_atoms.push_back(std::sqrt(current_sum_sq_ / sample_count));
        stats_atoms.push_back(std::sqrt(reference_sum_sq_ / sample_count));
        stats_atoms.push_back(std::sqrt(delta_sum_sq_ / sample_count));
        stats_atoms.push_back(delta_peak_);

        return stats_atoms;
    }

    void clear() {
        current_sum_sq_ = 0.0;
        reference_sum_sq_ = 0.0;
        delta_sum_sq_ = 0.0;
        delta_peak_ = 0.0;
        sample_count_ = 0;
    }

private:
    static double average_power(double left, double right) {
        return 0.5 * (left * left + right * right);
    }

    double current_sum_sq_ = 0.0;
    double reference_sum_sq_ = 0.0;
    double delta_sum_sq_ = 0.0;
    double delta_peak_ = 0.0;
    long sample_count_ = 0;
};

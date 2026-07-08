#pragma once

#include "c74_min.h"

#include <algorithm>
#include <array>

class AnalyzerCurveBatch {
public:
    static constexpr int max_output_points = 1024;

    int prepare(int bins_out) {
        if (last_pending_count_ != bins_out) {
            smoothing_initialized_ = false;
            last_pending_count_ = bins_out;
        }

        const int previous_pending_count = pending_count_;
        pending_count_ = bins_out;

        return previous_pending_count;
    }

    void store_bin(
        int output_index,
        int previous_pending_count,
        double raw_reference_db,
        double raw_target_db,
        double smoothing
    ) {
        const double reference_db = std::clamp(raw_reference_db, -120.0, 24.0);
        const double target_db = std::clamp(raw_target_db, -120.0, 24.0);
        const double difference_db = std::clamp(raw_target_db - raw_reference_db, -60.0, 60.0);

        if (!smoothing_initialized_ || output_index >= previous_pending_count) {
            smoothed_reference_[output_index] = reference_db;
            smoothed_target_[output_index] = target_db;
            smoothed_difference_[output_index] = difference_db;
        }
        else {
            smoothed_reference_[output_index] =
                smooth_toward(smoothed_reference_[output_index], reference_db, smoothing);

            smoothed_target_[output_index] =
                smooth_toward(smoothed_target_[output_index], target_db, smoothing);

            smoothed_difference_[output_index] =
                smooth_toward(smoothed_difference_[output_index], difference_db, smoothing);
        }

        pending_reference_[output_index] = smoothed_reference_[output_index];
        pending_target_[output_index] = smoothed_target_[output_index];
        pending_difference_[output_index] = smoothed_difference_[output_index];
    }

    void finalize_frame() {
        smoothing_initialized_ = true;
        has_pending_ = true;
    }

    void clear_pending() {
        has_pending_ = false;
    }

    bool has_pending() const {
        return has_pending_;
    }

    int pending_count() const {
        return pending_count_;
    }

    void send(
        c74::min::outlet<>& reference_out,
        c74::min::outlet<>& target_out,
        c74::min::outlet<>& difference_out
    ) const {
        c74::min::atoms reference_atoms;
        c74::min::atoms target_atoms;
        c74::min::atoms difference_atoms;

        for (int i = 0; i < pending_count_; ++i) {
            reference_atoms.push_back(pending_reference_[i]);
            target_atoms.push_back(pending_target_[i]);
            difference_atoms.push_back(pending_difference_[i]);
        }

        reference_out.send(reference_atoms);
        target_out.send(target_atoms);
        difference_out.send(difference_atoms);
    }

private:
    static double smooth_toward(double current, double target, double smoothing) {
        return current * smoothing + target * (1.0 - smoothing);
    }

    std::array<double, max_output_points> pending_reference_{};
    std::array<double, max_output_points> pending_target_{};
    std::array<double, max_output_points> pending_difference_{};
    std::array<double, max_output_points> smoothed_reference_{};
    std::array<double, max_output_points> smoothed_target_{};
    std::array<double, max_output_points> smoothed_difference_{};

    int pending_count_ = 0;
    int last_pending_count_ = 0;
    bool has_pending_ = false;
    bool smoothing_initialized_ = false;
};

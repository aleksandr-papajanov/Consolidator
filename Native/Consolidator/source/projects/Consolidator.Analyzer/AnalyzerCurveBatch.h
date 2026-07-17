#pragma once

#include "c74_min.h"

#include <algorithm>
#include <array>
#include <cmath>

class AnalyzerCurveBatch {
public:
    static constexpr int max_output_points = 1024;

    int prepare(int bins_out) {
        if (last_pending_count_ != bins_out) {
            smoothing_initialized_ = false;
            difference_smoothing_initialized_ = false;
            last_pending_count_ = bins_out;
        }

        const int previous_pending_count = pending_count_;
        pending_count_ = bins_out;

        return previous_pending_count;
    }

    void store_bin(
        int output_index,
        int previous_pending_count,
        double raw_current_db,
        double raw_reference_db,
        double smoothing,
        double low_frequency_amount,
        double spectrum_calibration_db,
        double spectrum_tilt_db
    ) {
        const double tilt_weight = spectrum_tilt_weight(output_index);
        const double tilt_offset = spectrum_tilt_db * tilt_weight;
        const double current_db = std::clamp(raw_current_db + spectrum_calibration_db + tilt_offset, -120.0, 48.0);
        const double reference_db = std::clamp(raw_reference_db + spectrum_calibration_db + tilt_offset, -120.0, 48.0);
        const double difference_db = std::clamp(raw_reference_db - raw_current_db, -60.0, 60.0);
        const double adaptive_smoothing = frequency_dependent_smoothing(
            output_index,
            smoothing,
            low_frequency_amount);

        if (!smoothing_initialized_ || output_index >= previous_pending_count) {
            smoothed_current_[output_index] = current_db;
            smoothed_reference_[output_index] = reference_db;
        }
        else {
            smoothed_current_[output_index] =
                smooth_toward(smoothed_current_[output_index], current_db, adaptive_smoothing);

            smoothed_reference_[output_index] =
                smooth_toward(smoothed_reference_[output_index], reference_db, adaptive_smoothing);
        }

        if (!difference_smoothing_initialized_ || output_index >= previous_pending_count) {
            smoothed_difference_[output_index] = difference_db;
        }
        else {
            smoothed_difference_[output_index] =
                smooth_toward(smoothed_difference_[output_index], difference_db, adaptive_smoothing);
        }

        pending_current_[output_index] = smoothed_current_[output_index];
        pending_reference_[output_index] = smoothed_reference_[output_index];
        pending_difference_[output_index] = smoothed_difference_[output_index];
    }

    void finalize_frame() {
        smoothing_initialized_ = true;
        difference_smoothing_initialized_ = true;
        has_pending_ = true;
    }

    void ResetDifference() {
        difference_smoothing_initialized_ = false;
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
        c74::min::outlet<>& current_out,
        c74::min::outlet<>& reference_out,
        c74::min::outlet<>& difference_out,
        bool send_difference
    ) const {
        c74::min::atoms current_atoms;
        c74::min::atoms reference_atoms;
        c74::min::atoms difference_atoms;

        for (int i = 0; i < pending_count_; ++i) {
            current_atoms.push_back(pending_current_[i]);
            reference_atoms.push_back(pending_reference_[i]);
            difference_atoms.push_back(pending_difference_[i]);
        }

        current_out.send(current_atoms);
        reference_out.send(reference_atoms);
        if (send_difference) {
            difference_out.send(difference_atoms);
        }
    }

private:
    static double smooth_toward(double current, double target, double smoothing) {
        return current * smoothing + target * (1.0 - smoothing);
    }

    double frequency_dependent_smoothing(
        int output_index,
        double smoothing,
        double low_frequency_amount
    ) const {
        if (pending_count_ <= 1) {
            return smoothing;
        }

        const double normalized = static_cast<double>(output_index) / static_cast<double>(pending_count_ - 1);
        const double low_frequency_weight = std::pow(std::max(0.0, 1.0 - normalized), 2.5);
        const double low_frequency_target = 0.9997;
        const double boosted = smoothing +
            (low_frequency_target - smoothing) * low_frequency_amount * low_frequency_weight;

        return std::clamp(boosted, 0.0, 0.9997);
    }

    double spectrum_tilt_weight(int output_index) const {
        if (pending_count_ <= 1) {
            return 0.0;
        }

        const double normalized = static_cast<double>(output_index) / static_cast<double>(pending_count_ - 1);
        return std::pow(std::max(0.0, normalized), 1.35);
    }

    std::array<double, max_output_points> pending_current_{};
    std::array<double, max_output_points> pending_reference_{};
    std::array<double, max_output_points> pending_difference_{};
    std::array<double, max_output_points> smoothed_current_{};
    std::array<double, max_output_points> smoothed_reference_{};
    std::array<double, max_output_points> smoothed_difference_{};

    int pending_count_ = 0;
    int last_pending_count_ = 0;
    bool has_pending_ = false;
    bool smoothing_initialized_ = false;
    bool difference_smoothing_initialized_ = false;
};

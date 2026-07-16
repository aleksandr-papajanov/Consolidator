#pragma once

#include "AnalyzerFrameBuffer.h"
#include "AnalyzerCurveBatch.h"
#include "EqFrequencyGrid.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

extern "C" {
#include "kiss_fft.h"
}

class AnalyzerSpectrumEngine {
public:
    void set_sample_rate(double sample_rate) {
        sample_rate_ = sample_rate;
    }

    int sanitized_fft_size(int value) const {
        if (value < 512) {
            value = 512;
        }

        if (value > AnalyzerFrameBuffer::max_fft_size) {
            value = AnalyzerFrameBuffer::max_fft_size;
        }

        return nearest_power_of_two(value);
    }

    void analyze(
        const AnalyzerFrameBuffer& frame,
        int fft_size,
        int bins_out,
        double smoothing,
        double low_frequency_amount,
        double spectrum_calibration_db,
        double spectrum_tilt_db,
        AnalyzerCurveBatch& curves
    ) const {
        const auto reference_spectrum = stereo_magnitude_db(
            frame.reference_left(),
            frame.reference_right(),
            frame.write_index(),
            fft_size);

        const auto current_spectrum = stereo_magnitude_db(
            frame.current_left(),
            frame.current_right(),
            frame.write_index(),
            fft_size);
        const int previous_pending_count = curves.prepare(bins_out);

        for (int i = 0; i < bins_out; ++i) {
            const int src_index = map_output_bin_to_fft_bin(i, bins_out, fft_size);

            curves.store_bin(
                i,
                previous_pending_count,
                current_spectrum[src_index],
                reference_spectrum[src_index],
                smoothing,
                low_frequency_amount,
                spectrum_calibration_db,
                spectrum_tilt_db);
        }

        curves.finalize_frame();
    }

private:
    static constexpr double pi = 3.1415926535897932384626433832795;

    static int nearest_power_of_two(int value) {
        int power = 1;

        while (power * 2 <= value) {
            power *= 2;
        }

        return power;
    }

    static double average_magnitude_db(double left_db, double right_db) {
        const double left_mag = std::pow(10.0, left_db / 20.0);
        const double right_mag = std::pow(10.0, right_db / 20.0);
        const double average_mag = 0.5 * (left_mag + right_mag);

        return 20.0 * std::log10(average_mag + 1e-12);
    }

    std::vector<double> stereo_magnitude_db(
        const std::array<double, AnalyzerFrameBuffer::max_fft_size>& left,
        const std::array<double, AnalyzerFrameBuffer::max_fft_size>& right,
        int write_index,
        int fft_size
    ) const {
        const auto left_db = magnitude_db(make_windowed_copy(left, write_index, fft_size), fft_size);
        const auto right_db = magnitude_db(make_windowed_copy(right, write_index, fft_size), fft_size);

        std::vector<double> stereo_db(fft_size / 2);

        for (int i = 0; i < fft_size / 2; ++i) {
            stereo_db[i] = average_magnitude_db(left_db[i], right_db[i]);
        }

        return stereo_db;
    }

    std::vector<double> make_windowed_copy(
        const std::array<double, AnalyzerFrameBuffer::max_fft_size>& source,
        int write_index,
        int fft_size
    ) const {
        std::vector<double> out(fft_size);
        const int start = write_index;

        for (int i = 0; i < fft_size; ++i) {
            const int index = (start + i) % fft_size;
            const double hann = 0.5 * (1.0 - std::cos((2.0 * pi * i) / (fft_size - 1)));

            out[i] = source[index] * hann;
        }

        return out;
    }

    std::vector<double> magnitude_db(const std::vector<double>& input, int fft_size) const {
        kiss_fft_cfg cfg = kiss_fft_alloc(fft_size, 0, nullptr, nullptr);

        std::vector<kiss_fft_cpx> in(fft_size);
        std::vector<kiss_fft_cpx> out(fft_size);

        for (int i = 0; i < fft_size; ++i) {
            in[i].r = static_cast<kiss_fft_scalar>(input[i]);
            in[i].i = 0;
        }

        kiss_fft(cfg, in.data(), out.data());
        kiss_fft_free(cfg);

        std::vector<double> db(fft_size / 2);
        const double coherent_gain = 0.5; // Hann window average gain
        const double amplitude_scale = static_cast<double>(fft_size) * coherent_gain * 0.5;

        for (int i = 0; i < fft_size / 2; ++i) {
            const double re = out[i].r;
            const double im = out[i].i;
            const double mag = std::sqrt(re * re + im * im);

            db[i] = 20.0 * std::log10((mag / amplitude_scale) + 1e-12);
        }

        return db;
    }

    int map_output_bin_to_fft_bin(int i, int bins_out, int fft_size) const {
        if (bins_out <= 1) {
            return 0;
        }

        const double normalized = static_cast<double>(i) / static_cast<double>(bins_out - 1);
        const int max_bin = (fft_size / 2) - 1;
        const double nyquist = sample_rate_ * 0.5;
        const double max_frequency = std::max(
            EqCurveGrid::min_hz,
            std::min(EqCurveGrid::max_hz, nyquist));
        const double frequency = EqCurveGrid::min_hz *
            std::pow(max_frequency / EqCurveGrid::min_hz, normalized);
        const double mapped_bin = frequency * static_cast<double>(fft_size) / sample_rate_;

        return std::clamp(static_cast<int>(std::round(mapped_bin)), 1, max_bin);
    }

    double sample_rate_ = EqCurveGrid::default_sample_rate;
};

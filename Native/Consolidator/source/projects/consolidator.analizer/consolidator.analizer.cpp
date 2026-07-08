#include "c74_min.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

extern "C" {
#include "kiss_fft.h"
}

using namespace c74::min;

class consolidator_analyzer :
    public object<consolidator_analyzer>,
    public sample_operator<4, 2> {
public:
    MIN_DESCRIPTION{ "Consolidator audio analyzer." };
    MIN_TAGS{ "audio, analyzer, fft" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> reference_l{ this, "(signal) reference left", "signal" };
    inlet<> reference_r{ this, "(signal) reference right", "signal" };
    inlet<> target_l{ this, "(signal) target left", "signal" };
    inlet<> target_r{ this, "(signal) target right", "signal" };

    outlet<> audio_l{ this, "(signal) passthrough left", "signal" };
    outlet<> audio_r{ this, "(signal) passthrough right", "signal" };

    outlet<> reference_out{ this, "(list) reference spectrum dB" };
    outlet<> target_out{ this, "(list) target spectrum dB" };
    outlet<> difference_out{ this, "(list) target-reference dB" };

    static constexpr int max_fft_size = 8192;
    static constexpr int max_output_points = 1024;

    struct StereoBuffer {
        std::array<double, max_fft_size> left{};
        std::array<double, max_fft_size> right{};

        void write(int index, sample left_sample, sample right_sample) {
            left[index] = left_sample;
            right[index] = right_sample;
        }
    };

    struct InputStats {
        double reference_sum_sq = 0.0;
        double target_sum_sq = 0.0;
        double delta_sum_sq = 0.0;
        double delta_peak = 0.0;
        long sample_count = 0;

        void clear() {
            reference_sum_sq = 0.0;
            target_sum_sq = 0.0;
            delta_sum_sq = 0.0;
            delta_peak = 0.0;
            sample_count = 0;
        }
    };

    attribute<int> fft_size_attr{
        this,
        "fftsize",
        2048,
        range { 512, max_fft_size },
        description { "FFT analysis window size in samples. Must be power of two." }
    };

    attribute<int> detail_attr{
        this,
        "detail",
        128,
        range { 32, max_output_points },
        description { "Number of output points sent to the UI." }
    };

    attribute<double> smoothing_attr{
        this,
        "smoothing",
        0.75,
        range { 0.0, 0.98 },
        description { "Temporal smoothing amount for output curves. 0 = none, 0.98 = very slow." }
    };

    StereoBuffer reference_buffer;
    StereoBuffer target_buffer;
    int write_index = 0;

    std::array<double, max_output_points> pending_reference{};
    std::array<double, max_output_points> pending_target{};
    std::array<double, max_output_points> pending_difference{};

    int pending_count = 0;
    int last_pending_count = 0;
    bool has_pending = false;

    std::array<double, max_output_points> smoothed_reference{};
    std::array<double, max_output_points> smoothed_target{};
    std::array<double, max_output_points> smoothed_difference{};

    bool smoothing_initialized = false;

    InputStats input_stats;
    double sample_rate = 44100.0;

    message<> dspsetup{ this, "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                sample_rate = static_cast<double>(args[0]);
            }

            return {};
        }
    };

    samples<2> operator()(sample ref_l, sample ref_r, sample eq_l, sample eq_r) {
        const int fft_size = sanitized_fft_size();
        const int bins_out = sanitized_detail(fft_size);

        record_input_statistics(ref_l, ref_r, eq_l, eq_r);
        write_input_frame(ref_l, ref_r, eq_l, eq_r);
        advance_analysis_frame(fft_size, bins_out);

        return { eq_l, eq_r };
    }

    void analyze_to_pending(int fft_size, int bins_out) {
        const auto reference_spectrum = stereo_magnitude_db(
            reference_buffer,
            fft_size);

        const auto target_spectrum = stereo_magnitude_db(
            target_buffer,
            fft_size);

        const int previous_pending_count = prepare_pending_frame(bins_out);

        for (int i = 0; i < bins_out; ++i) {
            const int src_index = map_output_bin_to_fft_bin(i, bins_out, fft_size);
            store_output_bin(
                i,
                previous_pending_count,
                reference_spectrum[src_index],
                target_spectrum[src_index]);
        }

        smoothing_initialized = true;
        has_pending = true;
    }

    message<> bang{ this, "bang", "Output latest analyzed curves.",
        MIN_FUNCTION {
            if (!has_pending) {
                return {};
            }

            send_pending_curves();
            has_pending = false;

            return {};
        }
    };

    message<> stats{ this, "stats", "Output input RMS diagnostics.",
        MIN_FUNCTION {
            difference_out.send(build_stats_atoms());
            clear_input_statistics();

            return {};
        }
    };

private:
    static constexpr double pi = 3.1415926535897932384626433832795;

    void record_input_statistics(sample ref_l, sample ref_r, sample eq_l, sample eq_r) {
        const double delta_l = eq_l - ref_l;
        const double delta_r = eq_r - ref_r;

        input_stats.reference_sum_sq += average_power(ref_l, ref_r);
        input_stats.target_sum_sq += average_power(eq_l, eq_r);
        input_stats.delta_sum_sq += average_power(delta_l, delta_r);
        input_stats.delta_peak =
            std::max(input_stats.delta_peak, std::max(std::abs(delta_l), std::abs(delta_r)));
        input_stats.sample_count++;
    }

    void write_input_frame(sample ref_l, sample ref_r, sample eq_l, sample eq_r) {
        reference_buffer.write(write_index, ref_l, ref_r);
        target_buffer.write(write_index, eq_l, eq_r);
    }

    void advance_analysis_frame(int fft_size, int bins_out) {
        write_index++;

        if (write_index < fft_size) {
            return;
        }

        analyze_to_pending(fft_size, bins_out);
        write_index = 0;
    }

    int prepare_pending_frame(int bins_out) {
        if (last_pending_count != bins_out) {
            smoothing_initialized = false;
            last_pending_count = bins_out;
        }

        const int previous_pending_count = pending_count;
        pending_count = bins_out;

        return previous_pending_count;
    }

    void store_output_bin(
        int output_index,
        int previous_pending_count,
        double raw_reference_db,
        double raw_target_db) {
        const double reference_db = std::clamp(raw_reference_db, -120.0, 24.0);
        const double target_db = std::clamp(raw_target_db, -120.0, 24.0);
        const double difference_db = std::clamp(raw_target_db - raw_reference_db, -60.0, 60.0);

        if (!smoothing_initialized || output_index >= previous_pending_count) {
            smoothed_reference[output_index] = reference_db;
            smoothed_target[output_index] = target_db;
            smoothed_difference[output_index] = difference_db;
        }
        else {
            const double smoothing = current_smoothing();

            smoothed_reference[output_index] =
                smooth_toward(smoothed_reference[output_index], reference_db, smoothing);

            smoothed_target[output_index] =
                smooth_toward(smoothed_target[output_index], target_db, smoothing);

            smoothed_difference[output_index] =
                smooth_toward(smoothed_difference[output_index], difference_db, smoothing);
        }

        pending_reference[output_index] = smoothed_reference[output_index];
        pending_target[output_index] = smoothed_target[output_index];
        pending_difference[output_index] = smoothed_difference[output_index];
    }

    void send_pending_curves() {
        atoms reference_atoms;
        atoms target_atoms;
        atoms difference_atoms;

        for (int i = 0; i < pending_count; ++i) {
            reference_atoms.push_back(pending_reference[i]);
            target_atoms.push_back(pending_target[i]);
            difference_atoms.push_back(pending_difference[i]);
        }

        reference_out.send(reference_atoms);
        target_out.send(target_atoms);
        difference_out.send(difference_atoms);
    }

    atoms build_stats_atoms() const {
        atoms stats_atoms;

        if (input_stats.sample_count <= 0) {
            stats_atoms.push_back(0.0);
            stats_atoms.push_back(0.0);
            stats_atoms.push_back(0.0);
            stats_atoms.push_back(0.0);
            return stats_atoms;
        }

        const double sample_count = static_cast<double>(input_stats.sample_count);

        stats_atoms.push_back(std::sqrt(input_stats.reference_sum_sq / sample_count));
        stats_atoms.push_back(std::sqrt(input_stats.target_sum_sq / sample_count));
        stats_atoms.push_back(std::sqrt(input_stats.delta_sum_sq / sample_count));
        stats_atoms.push_back(input_stats.delta_peak);

        return stats_atoms;
    }

    void clear_input_statistics() {
        input_stats.clear();
    }

    std::vector<double> stereo_magnitude_db(const StereoBuffer& buffer, int fft_size) {
        const auto left = make_windowed_copy(buffer.left, fft_size);
        const auto right = make_windowed_copy(buffer.right, fft_size);
        const auto left_db = magnitude_db(left, fft_size);
        const auto right_db = magnitude_db(right, fft_size);

        std::vector<double> stereo_db(fft_size / 2);

        for (int i = 0; i < fft_size / 2; ++i) {
            stereo_db[i] = average_magnitude_db(left_db[i], right_db[i]);
        }

        return stereo_db;
    }

    int sanitized_fft_size() const {
        int value = fft_size_attr;

        if (value < 512) {
            value = 512;
        }

        if (value > max_fft_size) {
            value = max_fft_size;
        }

        return nearest_power_of_two(value);
    }

    int sanitized_detail(int fft_size) const {
        int value = detail_attr;

        value = std::clamp(value, 32, max_output_points);

        const int max_bins = fft_size / 2;
        if (value > max_bins) {
            value = max_bins;
        }

        return value;
    }

    static int nearest_power_of_two(int value) {
        int power = 1;

        while (power * 2 <= value) {
            power *= 2;
        }

        return power;
    }

    double current_smoothing() const {
        return std::clamp(static_cast<double>(smoothing_attr), 0.0, 0.98);
    }

    static double average_power(double left, double right) {
        return 0.5 * (left * left + right * right);
    }

    static double smooth_toward(double current, double target, double smoothing) {
        return current * smoothing + target * (1.0 - smoothing);
    }

    int map_output_bin_to_fft_bin(int i, int bins_out, int fft_size) const {
        if (bins_out <= 1) {
            return 0;
        }

        const double normalized = static_cast<double>(i) / static_cast<double>(bins_out - 1);
        const int max_bin = (fft_size / 2) - 1;
        const double min_frequency = 20.0;
        const double nyquist = sample_rate * 0.5;
        const double min_bin = (min_frequency / nyquist) * max_bin;
        const double log_min = std::log(std::max(1.0, min_bin));
        const double log_max = std::log(static_cast<double>(max_bin));
        const double mapped_bin = std::exp(log_min + normalized * (log_max - log_min));

        return std::clamp(static_cast<int>(std::round(mapped_bin)), 1, max_bin);
    }

    static double average_magnitude_db(double left_db, double right_db) {
        const double left_mag = std::pow(10.0, left_db / 20.0);
        const double right_mag = std::pow(10.0, right_db / 20.0);
        const double average_mag = 0.5 * (left_mag + right_mag);

        return 20.0 * std::log10(average_mag + 1e-12);
    }

    std::vector<double> make_windowed_copy(
        const std::array<double, max_fft_size>& source,
        int fft_size) {
        std::vector<double> out(fft_size);

        const int start = write_index;

        for (int i = 0; i < fft_size; ++i) {
            const int index = (start + i) % fft_size;
            const double hann = 0.5 * (1.0 - std::cos((2.0 * pi * i) / (fft_size - 1)));

            out[i] = source[index] * hann;
        }

        return out;
    }

    std::vector<double> magnitude_db(const std::vector<double>& input, int fft_size) {
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

        for (int i = 0; i < fft_size / 2; ++i) {
            const double re = out[i].r;
            const double im = out[i].i;
            const double mag = std::sqrt(re * re + im * im);

            db[i] = 20.0 * std::log10(mag + 1e-12);
        }

        return db;
    }
};

MIN_EXTERNAL(consolidator_analyzer);

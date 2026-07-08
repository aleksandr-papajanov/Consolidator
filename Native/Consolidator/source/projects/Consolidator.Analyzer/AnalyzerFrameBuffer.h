#pragma once

#include "c74_min.h"

#include <array>

struct StereoSample {
    c74::min::sample left = 0.0;
    c74::min::sample right = 0.0;
};

struct AnalyzerInputFrame {
    StereoSample reference;
    StereoSample target;
};

class AnalyzerFrameBuffer {
public:
    static constexpr int max_fft_size = 8192;

    void write(const AnalyzerInputFrame& frame) {
        reference_left_[write_index_] = frame.reference.left;
        reference_right_[write_index_] = frame.reference.right;
        target_left_[write_index_] = frame.target.left;
        target_right_[write_index_] = frame.target.right;
    }

    bool advance(int fft_size) {
        ++write_index_;
        return write_index_ >= fft_size;
    }

    void reset() {
        write_index_ = 0;
    }

    int write_index() const {
        return write_index_;
    }

    const std::array<double, max_fft_size>& reference_left() const {
        return reference_left_;
    }

    const std::array<double, max_fft_size>& reference_right() const {
        return reference_right_;
    }

    const std::array<double, max_fft_size>& target_left() const {
        return target_left_;
    }

    const std::array<double, max_fft_size>& target_right() const {
        return target_right_;
    }

private:
    std::array<double, max_fft_size> reference_left_{};
    std::array<double, max_fft_size> reference_right_{};
    std::array<double, max_fft_size> target_left_{};
    std::array<double, max_fft_size> target_right_{};
    int write_index_ = 0;
};

#pragma once

#include "AudioBlockView.h"

#include <cstddef>
#include <span>
#include <stdexcept>

namespace consolidator::audio {

class StereoBufferView {
public:
    StereoBufferView(std::span<double> left, std::span<double> right)
        : left(left), right(right) {
        if (left.size() != right.size()) {
            throw std::invalid_argument("Stereo channels must have the same sample count");
        }
    }

    AudioBlockView Left() const {
        return AudioBlockView{ left };
    }

    AudioBlockView Right() const {
        return AudioBlockView{ right };
    }

    std::size_t SampleCount() const {
        return left.size();
    }

private:
    std::span<double> left;
    std::span<double> right;
};

} // namespace consolidator::audio

#pragma once

#include <cstddef>
#include <span>

namespace consolidator::audio {

class AudioBlockView {
public:
    explicit AudioBlockView(std::span<double> samples)
        : samples(samples) {}

    std::span<double> Samples() const {
        return samples;
    }

    std::size_t SampleCount() const {
        return samples.size();
    }

private:
    std::span<double> samples;
};

} // namespace consolidator::audio

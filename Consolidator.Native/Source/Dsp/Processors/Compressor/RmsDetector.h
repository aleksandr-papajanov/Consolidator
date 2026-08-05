#pragma once

#include <array>
#include <cstddef>

namespace consolidator::dsp
{

class RmsDetector
{
public:
    static constexpr std::size_t kWindowSize = 64;

    [[nodiscard]] double ProcessSample(double sample) noexcept;

    void Reset() noexcept;

private:
    std::array<double, kWindowSize> squaredSamples_{};

    std::size_t writeIndex_ = 0;
    std::size_t sampleCount_ = 0;

    double squaredSum_ = 0.0;
};

} // namespace consolidator::dsp
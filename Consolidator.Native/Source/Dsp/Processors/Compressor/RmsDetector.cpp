#include "Dsp/Processors/Compressor/RmsDetector.h"

#include <algorithm>
#include <cmath>

namespace consolidator::dsp
{

double RmsDetector::ProcessSample(double sample) noexcept
{
    const double squaredSample = sample * sample;

    squaredSum_ -= squaredSamples_[writeIndex_];
    squaredSamples_[writeIndex_] = squaredSample;
    squaredSum_ += squaredSample;

    writeIndex_ = (writeIndex_ + 1) % kWindowSize;

    if (sampleCount_ < kWindowSize)
    {
        ++sampleCount_;
    }

    if (sampleCount_ == 0)
    {
        return 0.0;
    }

    const double meanSquare = squaredSum_ / static_cast<double>(sampleCount_);

    const double levelLinear = std::sqrt(std::max(meanSquare, 0.0));

    meterState_.levelLinear.store(
        static_cast<float>(levelLinear),
        std::memory_order_relaxed);

    return levelLinear;
}

void RmsDetector::Reset() noexcept
{
    squaredSamples_.fill(0.0);

    writeIndex_ = 0;
    sampleCount_ = 0;
    squaredSum_ = 0.0;
    meterState_.levelLinear.store(0.0f, std::memory_order_relaxed);
}

} // namespace consolidator::dsp

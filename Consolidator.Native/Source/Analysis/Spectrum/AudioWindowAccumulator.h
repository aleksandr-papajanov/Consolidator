#pragma once

#include <cstddef>
#include <cstdint>

#include "Analysis/Spectrum/SpectrumTypes.h"

namespace consolidator::analysis
{

// Accumulates stereo audio blocks into fixed-size mono analysis windows.
class AudioWindowAccumulator final
{
public:
    void Prepare(double sampleRate) noexcept { sampleRate_ = sampleRate; }

    template <typename OnWindow>
    void Push(const double* left, const double* right,
              std::size_t frameCount, OnWindow&& onWindow) noexcept
    {
        for (std::size_t frame = 0; frame < frameCount; ++frame)
        {
            window_.samples[accumulatedFrames_++] = static_cast<float>(
                0.5 * (left[frame] + right[frame]));
            if (accumulatedFrames_ != kFftSize)
                continue;

            window_.sampleRate = sampleRate_;
            window_.revision = nextRevision_++;
            onWindow(window_);
            accumulatedFrames_ = 0;
        }
    }

private:
    AudioWindow window_{};
    double sampleRate_ = 0.0;
    std::size_t accumulatedFrames_ = 0;
    std::uint64_t nextRevision_ = 1;
};

} // namespace consolidator::analysis

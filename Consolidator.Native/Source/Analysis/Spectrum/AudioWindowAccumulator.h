#pragma once

#include <cstddef>
#include <cstdint>

#include "Analysis/Spectrum/SpectrumTypes.h"

namespace consolidator::analysis
{

// Accumulates stereo audio blocks into fixed-size analysis windows.
class AudioWindowAccumulator final
{
  public:
    void Prepare(double sampleRate) noexcept { sampleRate_ = sampleRate; }

    // Invalidates the partial window on the audio thread.
    void Reset() noexcept
    {
        accumulatedFrames_ = 0;
    }

    template <typename OnWindow>
    void Push(
        const double* left,
        const double* right,
        std::size_t frameCount,
        std::uint64_t generation,
        OnWindow&& onWindow) noexcept
    {
        for (std::size_t frame = 0; frame < frameCount; ++frame)
        {
            window_.leftSamples[accumulatedFrames_] = static_cast<float>(
                left[frame]);
            window_.rightSamples[accumulatedFrames_++] = static_cast<float>(
                right[frame]);
            if (accumulatedFrames_ != kFftSize)
            {
                continue;
            }

            window_.sampleRate = sampleRate_;
            window_.revision = nextRevision_++;
            window_.generation = generation;
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

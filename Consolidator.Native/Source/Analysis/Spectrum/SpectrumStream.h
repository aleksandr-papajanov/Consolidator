#pragma once

#include <cstddef>
#include <cstdint>
#include <atomic>

#include "Analysis/LatestSnapshot.h"
#include "Analysis/LatestValue.h"
#include "Analysis/Spectrum/AudioWindowAccumulator.h"
#include "Analysis/Spectrum/SpectrumTypes.h"

namespace consolidator::analysis
{

// Owns one input-to-spectrum latest-value pipeline for one audio stream.
class SpectrumStream final
{
  public:
    void PushAudio(
        const double* left,
        const double* right,
        std::size_t frameCount) noexcept;

    [[nodiscard]] bool TryConsumeInput(AudioWindow& window) noexcept;

    void PublishOutput(const SpectrumSnapshot& snapshot) noexcept;

    [[nodiscard]] bool ReadLatestOutput(
        SpectrumSnapshot& snapshot) const noexcept;

    void Prepare(double sampleRate) noexcept;

    // Requests an audio-thread-only reset before the next accumulation.
    void Reset() noexcept;

  private:
    AudioWindowAccumulator accumulator_;
    LatestSnapshot<AudioWindow> input_;
    LatestValue<SpectrumSnapshot> output_;
    std::uint64_t processedRevision_ = 0;
    std::atomic_bool resetRequested_{false};
    std::atomic_uint64_t generation_{0};
};

} // namespace consolidator::analysis

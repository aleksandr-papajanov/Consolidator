#pragma once

#include <cstddef>
#include <cstdint>

#include "Analysis/LatestSnapshot.h"
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

    [[nodiscard]] bool TryReadOutput(
        SpectrumSnapshot& snapshot,
    std::uint64_t& revision) const noexcept;

    void Prepare(double sampleRate) noexcept;

private:
    AudioWindowAccumulator accumulator_;
    LatestSnapshot<AudioWindow> input_;
    LatestSnapshot<SpectrumSnapshot> output_;
    std::uint64_t processedRevision_ = 0;
};

} // namespace consolidator::analysis

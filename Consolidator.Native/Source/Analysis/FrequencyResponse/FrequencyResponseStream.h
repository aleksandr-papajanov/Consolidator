#pragma once

#include <cstdint>

#include "Analysis/LatestValue.h"
#include "Analysis/FrequencyResponse/FrequencyResponseTypes.h"

namespace consolidator::analysis
{

// Owns one latest-value request/result pipeline for theoretical response work.
class FrequencyResponseStream final
{
  public:
    void PublishOutput(const EqualizerCurveSnapshot& snapshot) noexcept;
    [[nodiscard]] bool ReadLatestOutput(
        EqualizerCurveSnapshot& snapshot) const noexcept;

    [[nodiscard]] bool NeedsProcessing(
        std::uint64_t inputRevision,
        std::uint64_t viewRevision) const noexcept;

    void MarkProcessed(
        std::uint64_t inputRevision,
        std::uint64_t viewRevision) noexcept;

  private:
    LatestValue<EqualizerCurveSnapshot> output_;
    std::uint64_t processedInputRevision_ = 0;
    std::uint64_t processedViewRevision_ = 0;
};

} // namespace consolidator::analysis

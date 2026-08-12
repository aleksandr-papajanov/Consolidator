#pragma once

#include <cstdint>

#include "Analysis/LatestSnapshot.h"
#include "Analysis/FrequencyResponse/FrequencyResponseTypes.h"

namespace consolidator::analysis
{

// Owns one latest-value request/result pipeline for theoretical response work.
class FrequencyResponseStream final
{
public:
    void PublishRequest(const FrequencyResponseRequest& request) noexcept;
    [[nodiscard]] bool TryConsumeRequest(FrequencyResponseRequest& request) noexcept;
    void PublishOutput(const FrequencyResponseSnapshot& snapshot) noexcept;
    [[nodiscard]] bool TryReadOutput(
        FrequencyResponseSnapshot& snapshot,
        std::uint64_t& revision) const noexcept;

private:
    LatestSnapshot<FrequencyResponseRequest> input_;
    LatestSnapshot<FrequencyResponseSnapshot> output_;
    std::uint64_t processedRevision_ = 0;
};

} // namespace consolidator::analysis

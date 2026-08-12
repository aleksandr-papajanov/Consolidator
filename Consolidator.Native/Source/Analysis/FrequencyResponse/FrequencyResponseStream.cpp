#include "Analysis/FrequencyResponse/FrequencyResponseStream.h"

namespace consolidator::analysis
{

void FrequencyResponseStream::PublishOutput(
    const EqualizerCurveSnapshot& snapshot) noexcept
{
    output_.Publish(snapshot);
}

bool FrequencyResponseStream::ReadLatestOutput(
    EqualizerCurveSnapshot& snapshot) const noexcept
{
    return output_.ReadLatest(snapshot);
}

bool FrequencyResponseStream::NeedsProcessing(
    std::uint64_t inputRevision,
    std::uint64_t viewRevision) const noexcept
{
    return inputRevision != processedInputRevision_ ||
        viewRevision != processedViewRevision_;
}

void FrequencyResponseStream::MarkProcessed(
    std::uint64_t inputRevision,
    std::uint64_t viewRevision) noexcept
{
    processedInputRevision_ = inputRevision;
    processedViewRevision_ = viewRevision;
}

} // namespace consolidator::analysis

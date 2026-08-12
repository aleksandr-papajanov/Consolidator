#include "Analysis/FrequencyResponse/FrequencyResponseStream.h"

namespace consolidator::analysis
{

void FrequencyResponseStream::PublishOutput(
    const EqualizerCurveSnapshot& snapshot)
{
    output_.Publish(snapshot);
}

bool FrequencyResponseStream::ReadLatestOutput(
    EqualizerCurveSnapshot& snapshot,
    std::uint64_t lastRevision) const
{
    return output_.TryReadNewerThan(snapshot, lastRevision);
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

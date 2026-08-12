#include "Analysis/FrequencyResponse/FrequencyResponseStream.h"

namespace consolidator::analysis
{

void FrequencyResponseStream::PublishRequest(
    const FrequencyResponseRequest& request) noexcept
{
    input_.Publish(request);
}

bool FrequencyResponseStream::TryConsumeRequest(
    FrequencyResponseRequest& request) noexcept
{
    if (!input_.TryReadNewerThan(request, processedRevision_))
        return false;

    processedRevision_ = request.revision;
    return true;
}

void FrequencyResponseStream::PublishOutput(
    const FrequencyResponseSnapshot& snapshot) noexcept
{
    output_.Publish(snapshot);
}

bool FrequencyResponseStream::TryReadOutput(
    FrequencyResponseSnapshot& snapshot,
    std::uint64_t& revision) const noexcept
{
    if (!output_.TryReadNewerThan(snapshot, revision))
        return false;

    revision = snapshot.revision;
    return true;
}

} // namespace consolidator::analysis

#include "Analysis/Spectrum/SpectrumStream.h"

namespace consolidator::analysis
{

void SpectrumStream::Prepare(double sampleRate) noexcept
{
    accumulator_.Prepare(sampleRate);
}

void SpectrumStream::PushAudio(
    const double* left,
    const double* right,
    std::size_t frameCount) noexcept
{
    accumulator_.Push(
        left,
        right,
        frameCount,
        [this](const AudioWindow& window) noexcept
        {
            input_.Publish(window);
        });
}

bool SpectrumStream::TryConsumeInput(AudioWindow& window) noexcept
{
    if (!input_.TryReadNewerThan(window, processedRevision_))
    {
        return false;
    }

    processedRevision_ = window.revision;
    return true;
}

void SpectrumStream::PublishOutput(
    const SpectrumSnapshot& snapshot) noexcept
{
    output_.Publish(snapshot);
}

bool SpectrumStream::TryReadOutput(
    SpectrumSnapshot& snapshot,
    std::uint64_t& revision) const noexcept
{
    if (!output_.TryReadNewerThan(snapshot, revision))
    {
        return false;
    }
    revision = snapshot.revision;
    return true;
}

} // namespace consolidator::analysis

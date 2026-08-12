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
    if (resetRequested_.exchange(false, std::memory_order_acq_rel))
    {
        accumulator_.Reset();
    }

    accumulator_.Push(
        left, right, frameCount,
        generation_.load(std::memory_order_relaxed),
        [this](const AudioWindow& window) noexcept
        {
            input_.Publish(window);
        });
}

bool SpectrumStream::TryConsumeInput(AudioWindow& window) noexcept
{
    if (resetRequested_.load(std::memory_order_acquire))
    {
        return false;
    }

    if (!input_.TryReadNewerThan(window, processedRevision_))
    {
        return false;
    }

    if (window.generation != generation_.load(std::memory_order_acquire))
    {
        return false;
    }

    processedRevision_ = window.revision;
    return true;
}

void SpectrumStream::Reset() noexcept
{
    generation_.fetch_add(1, std::memory_order_acq_rel);
    resetRequested_.store(true, std::memory_order_release);
}

void SpectrumStream::PublishOutput(
    const SpectrumSnapshot& snapshot) noexcept
{
    output_.Publish(snapshot);
}

bool SpectrumStream::ReadLatestOutput(SpectrumSnapshot& snapshot) const noexcept
{
    return output_.ReadLatest(snapshot);
}

} // namespace consolidator::analysis

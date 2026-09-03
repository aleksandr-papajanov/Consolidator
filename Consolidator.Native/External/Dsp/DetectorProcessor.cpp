#include "DspProcessors.h"

#include <algorithm>

namespace consolidator::max::dsp
{

DetectorProcessor::DetectorProcessor(std::size_t filterOffset) noexcept
    : filterOffset_(filterOffset)
{
}

void DetectorProcessor::Prepare(
    const DspProcessingContext& context)
{
    left_.assign(context.maximumFrameCount, 0.0);
    right_.assign(context.maximumFrameCount, 0.0);
}

void DetectorProcessor::Reset() noexcept
{
    std::fill(left_.begin(), left_.end(), 0.0);
    std::fill(right_.begin(), right_.end(), 0.0);
}

void DetectorProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    for (std::size_t index = 0; index < kFilterCount; ++index)
    {
        filters_[index] = snapshot.detectorFilters[filterOffset_ + index];
    }
}

void DetectorProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    const auto frameCount = frame.main.frameCount;
    if (frameCount > left_.size() || frameCount > right_.size())
    {
        return;
    }

    if (frame.main.left != nullptr)
    {
        std::copy_n(frame.main.left, frameCount, left_.data());
    }
    if (frame.main.right != nullptr)
    {
        std::copy_n(frame.main.right, frameCount, right_.data());
    }
    frame.detectorSignal = {
        frame.main.left == nullptr ? nullptr : left_.data(),
        frame.main.right == nullptr ? nullptr : right_.data()
    };
}

} // namespace consolidator::max::dsp

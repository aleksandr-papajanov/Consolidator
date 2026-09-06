#include "DspParallelStage.h"

#include <algorithm>
#include <cstddef>

#include "DspProcessingPrimitives.h"

namespace consolidator::max::dsp
{

void DspParallelStage::Prepare(const DspProcessingContext& context)
{
    branchLeft_.assign(context.maximumFrameCount, 0.0);
    branchRight_.assign(context.maximumFrameCount, 0.0);
    wet_.Prepare(context);
}

void DspParallelStage::Reset() noexcept
{
    std::fill(branchLeft_.begin(), branchLeft_.end(), 0.0);
    std::fill(branchRight_.begin(), branchRight_.end(), 0.0);
    wet_.Reset();
    wasProcessing_ = false;
}

void DspParallelStage::SetWetTarget(double wet) noexcept
{
    wet_.SetTarget(std::clamp(wet, 0.0, 1.0));
}

void DspParallelStage::Process(
    DspProcessingFrame& frame,
    DspProcessor& processor) noexcept
{
    auto& main = frame.main;
    if (wet_.CurrentValue() == 0.0 && !wet_.IsSmoothing())
    {
        if (wasProcessing_)
        {
            processor.Reset();
            wasProcessing_ = false;
        }
        return;
    }
    wasProcessing_ = true;
    if (main.frameCount > branchLeft_.size() ||
        main.frameCount > branchRight_.size())
    {
        return;
    }

    // At stable 100% wet the dry copy and interpolation have no purpose.
    // Keep detector routing local, as with the copied branch below.
    if (wet_.CurrentValue() == 1.0 && !wet_.IsSmoothing())
    {
        auto wetFrame = frame;
        processor.Process(wetFrame);
        return;
    }

    if (main.left != nullptr)
    {
        std::copy_n(main.left, main.frameCount, branchLeft_.data());
    }
    if (main.right != nullptr)
    {
        std::copy_n(main.right, main.frameCount, branchRight_.data());
    }
    DspAudioBuffer branch{
        main.left == nullptr ? nullptr : branchLeft_.data(),
        main.right == nullptr ? nullptr : branchRight_.data(),
        main.frameCount
    };
    DspProcessingFrame branchFrame{
        branch,
        frame.detectorSignal,
        frame.plan,
        frame.policy
    };
    processor.Process(branchFrame);
    const auto channels = SelectChannels(main, frame.plan.channelMode);
    for (std::size_t index = 0; index < main.frameCount; ++index)
    {
        const auto wet = wet_.NextValue();
        if (channels.left != nullptr)
        {
            main.left[index] += wet * (branch.left[index] - main.left[index]);
        }
        if (channels.right != nullptr)
        {
            main.right[index] += wet * (branch.right[index] - main.right[index]);
        }
    }
}

} // namespace consolidator::max::dsp

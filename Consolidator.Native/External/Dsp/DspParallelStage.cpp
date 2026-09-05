#include "DspParallelStage.h"

#include <algorithm>


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
    for (std::size_t index = 0; index < main.frameCount; ++index)
    {
        const auto wet = wet_.NextValue();
        if (frame.plan.channelMode != DspChannelMode::Right && main.left != nullptr)
        {
            main.left[index] += wet * (branch.left[index] - main.left[index]);
        }
        if (frame.plan.channelMode != DspChannelMode::Left && main.right != nullptr)
        {
            main.right[index] += wet * (branch.right[index] - main.right[index]);
        }
    }
}

} // namespace consolidator::max::dsp

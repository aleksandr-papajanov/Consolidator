#include "DspParallelStage.h"

#include <algorithm>

#include "DspProcessingPrimitives.h"

namespace consolidator::max::dsp
{

void DspParallelStage::Prepare(const DspProcessingContext& context)
{
    branchLeft_.assign(context.maximumFrameCount, 0.0);
    branchRight_.assign(context.maximumFrameCount, 0.0);
}

void DspParallelStage::Reset() noexcept
{
    std::fill(branchLeft_.begin(), branchLeft_.end(), 0.0);
    std::fill(branchRight_.begin(), branchRight_.end(), 0.0);
}

void DspParallelStage::Process(
    DspProcessingFrame& frame,
    DspProcessor& processor) noexcept
{
    auto& main = frame.main;
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
    DspParallelMixer::Add(main, branch, frame.plan.channelMode);
}

} // namespace consolidator::max::dsp

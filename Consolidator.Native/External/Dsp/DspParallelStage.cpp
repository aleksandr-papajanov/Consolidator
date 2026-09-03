#include "DspParallelStage.h"

#include <algorithm>

namespace consolidator::max::dsp
{

void DspParallelStage::Prepare(const DspProcessingContext& context)
{
    branchLeft_.assign(context.maximumFrameCount, 0.0);
    branchRight_.assign(context.maximumFrameCount, 0.0);
    enabled_ = false;
}

void DspParallelStage::Reset() noexcept
{
    enabled_ = false;
}

void DspParallelStage::SetEnabled(bool enabled) noexcept
{
    enabled_ = enabled;
}

void DspParallelStage::Process(
    DspProcessingFrame& frame) noexcept
{
    auto& main = frame.main;
    if (!enabled_ ||
        main.frameCount > branchLeft_.size() ||
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
    ProcessBranch(
        branchLeft_.data(),
        branchRight_.data(),
        frame.externalSidechain,
        main.frameCount);

    for (std::size_t index = 0; index < main.frameCount; ++index)
    {
        if (main.left != nullptr)
        {
            main.left[index] = (main.left[index] + branchLeft_[index]) * 0.5;
        }
        if (main.right != nullptr)
        {
            main.right[index] = (main.right[index] + branchRight_[index]) * 0.5;
        }
    }
}

void DspParallelStage::ProcessBranch(
    double* left,
    double* right,
    const DspSidechainBuffer& sidechain,
    std::size_t frameCount) noexcept
{
    (void)left;
    (void)right;
    (void)sidechain;
    (void)frameCount;
}

} // namespace consolidator::max::dsp

#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void LevelerProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    (void)context;
}

void LevelerProcessor::Reset() noexcept
{
}

void LevelerProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    targetDb_ = snapshot.inputTarget;
}

void LevelerProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    (void)frame;
}

} // namespace consolidator::max::dsp

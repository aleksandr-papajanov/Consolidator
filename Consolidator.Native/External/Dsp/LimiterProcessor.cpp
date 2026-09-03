#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void LimiterProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    (void)context;
}

void LimiterProcessor::Reset() noexcept
{
}

void LimiterProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    targetDb_ = snapshot.outputTarget;
}

void LimiterProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    (void)frame;
}

} // namespace consolidator::max::dsp

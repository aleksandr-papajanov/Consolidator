#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void LimiterProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    targetDb_.Prepare(context);
}

void LimiterProcessor::Reset() noexcept
{
    targetDb_.Reset();
}

void LimiterProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    targetDb_.SetTarget(snapshot.outputTarget);
}

void LimiterProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    targetDb_.Advance(frame.main.frameCount);
}

} // namespace consolidator::max::dsp

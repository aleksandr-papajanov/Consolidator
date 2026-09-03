#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void LevelerProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    targetDb_.Prepare(context);
}

void LevelerProcessor::Reset() noexcept
{
    targetDb_.Reset();
}

void LevelerProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    targetDb_.SetTarget(snapshot.inputTarget);
}

void LevelerProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    targetDb_.Advance(frame.main.frameCount);
}

} // namespace consolidator::max::dsp

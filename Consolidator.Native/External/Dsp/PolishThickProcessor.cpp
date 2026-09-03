#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void PolishThickProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    amount_.Prepare(context);
}

void PolishThickProcessor::Reset() noexcept
{
    amount_.Reset();
}

void PolishThickProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    amount_.SetTarget(snapshot.polishThick);
}

void PolishThickProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    amount_.Advance(frame.main.frameCount);
}

} // namespace consolidator::max::dsp

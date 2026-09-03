#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void PolishAirProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    amount_.Prepare(context);
}

void PolishAirProcessor::Reset() noexcept
{
    amount_.Reset();
}

void PolishAirProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    amount_.SetTarget(snapshot.polishAir);
}

void PolishAirProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    amount_.Advance(frame.main.frameCount);
}

} // namespace consolidator::max::dsp

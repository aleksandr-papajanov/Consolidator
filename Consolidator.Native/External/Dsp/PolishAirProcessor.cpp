#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void PolishAirProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    (void)context;
}

void PolishAirProcessor::Reset() noexcept
{
}

void PolishAirProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    amount_ = snapshot.polishAir;
}

void PolishAirProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    (void)frame;
}

} // namespace consolidator::max::dsp

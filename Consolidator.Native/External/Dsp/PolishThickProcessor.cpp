#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void PolishThickProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    (void)context;
}

void PolishThickProcessor::Reset() noexcept
{
}

void PolishThickProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    amount_ = snapshot.polishThick;
}

void PolishThickProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    (void)frame;
}

} // namespace consolidator::max::dsp

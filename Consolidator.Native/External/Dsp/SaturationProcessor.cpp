#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void SaturationProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    (void)context;
}

void SaturationProcessor::Reset() noexcept
{
}

void SaturationProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    drive_ = snapshot.saturatorDrive;
    curve_ = snapshot.saturatorCurve;
    split_ = snapshot.saturatorSplit != 0;
}

void SaturationProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    (void)frame;
}

} // namespace consolidator::max::dsp

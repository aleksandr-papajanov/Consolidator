#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void SaturationProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    drive_.Prepare(context);
    curve_.Prepare(context);
}

void SaturationProcessor::Reset() noexcept
{
    drive_.Reset();
    curve_.Reset();
}

void SaturationProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    drive_.SetTarget(snapshot.saturatorDrive);
    curve_.SetTarget(snapshot.saturatorCurve);
    split_ = snapshot.saturatorSplit != 0;
}

void SaturationProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    drive_.Advance(frame.main.frameCount);
    curve_.Advance(frame.main.frameCount);
}

} // namespace consolidator::max::dsp

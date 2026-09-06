#include "SaturatorProcessor.h"

namespace consolidator::max::dsp
{

void SaturatorProcessor::Prepare(const DspProcessingContext& context)
{
    detector_.Prepare(context);
    saturation_.Prepare(context);
    output_.Prepare(context);
}

void SaturatorProcessor::Reset() noexcept
{
    detector_.Reset();
    saturation_.Reset();
    output_.Reset();
}

void SaturatorProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    detector_.SetParameters(snapshot);
    saturation_.SetParameters(snapshot);
    output_.SetTargetDb(snapshot.saturatorOutputDb);
}

void SaturatorProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    detector_.Process(frame);
    saturation_.Process(frame);
    output_.Process(frame.main, frame.plan.channelMode);
}

} // namespace consolidator::max::dsp

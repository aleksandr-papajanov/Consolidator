#include "OutputProcessor.h"

namespace consolidator::max::dsp
{

void OutputProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    level_.Prepare(context);
    limiter_.Prepare(context);
}

void OutputProcessor::Reset() noexcept
{
    level_.Reset();
    limiter_.Reset();
}

void OutputProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    level_.SetTargetDb(snapshot.outputLevel);
    limiter_.SetParameters(snapshot);
}

void OutputProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    level_.Process(frame.main, frame.plan.channelMode);
    if (limiter_.RequiresProcessing())
    {
        limiter_.Process(frame);
    }
}

bool OutputProcessor::RequiresProcessing() const noexcept
{
    return level_.RequiresProcessing() || limiter_.RequiresProcessing();
}

} // namespace consolidator::max::dsp
